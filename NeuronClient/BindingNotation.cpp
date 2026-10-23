#include "pch.h"

#include "AuthoringParser.h"
#include "BindingNotation.h"
#include "InputNames.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <optional>
#include <utility>

namespace Neuron
{

namespace
{

// INP-071's node-kind properties.
constexpr std::array NODE_KINDS{std::string_view{"key"},     std::string_view{"mouse"}, std::string_view{"pad"},
                                std::string_view{"all"},     std::string_view{"chord"}, std::string_view{"not"},
                                std::string_view{"alias"},   std::string_view{"latch"}, std::string_view{"idle"},
                                std::string_view{"setting"}, std::string_view{"value"}};

using Parsed = std::expected<InputBinding, std::string>;

std::unexpected<std::string> Refuse(std::string_view _path, std::string_view _what)
{
  return std::unexpected(std::format("{}: {}", _path, _what));
}

const JsonValue* MemberOf(const JsonValue& _object, std::string_view _key) noexcept
{
  const auto found = std::ranges::find(_object.members, _key, &JsonMember::key);
  return found == _object.members.end() ? nullptr : &found->value;
}

// Every member of the node is its kind or one of _allowed.
std::optional<std::string> UnknownProperty(const JsonValue& _object, std::string_view _kind,
                                           std::initializer_list<std::string_view> _allowed)
{
  for (const JsonMember& member : _object.members)
  {
    if (member.key != _kind && std::ranges::find(_allowed, member.key) == _allowed.end())
    {
      return member.key;
    }
  }
  return std::nullopt;
}

Parsed NodeOf(const JsonValue& _value, const std::string& _path, bool _chordPart);

Parsed LeafOf(const JsonValue& _node, std::string_view _kind, const std::string& _path, bool _chordPart)
{
  const JsonValue& name = *MemberOf(_node, _kind);
  if (name.kind != JsonKind::String)
  {
    return Refuse(_path, std::format("{} is not a control name", _kind));
  }
  InputBinding binding;
  if (_kind == "key")
  {
    const std::optional<KeyCode> key = KeyOfName(name.text);
    if (!key.has_value())
    {
      return Refuse(_path, std::format("{} is not a key name", name.text));
    }
    binding.leaf = {InputDevice::Key, static_cast<std::uint16_t>(*key), Condition::Held};
  }
  else
  {
    const std::optional<MouseControl> control = MouseControlOfName(name.text);
    if (!control.has_value())
    {
      return Refuse(_path, std::format("{} is not a mouse control name", name.text));
    }
    binding.leaf = {InputDevice::Mouse, static_cast<std::uint16_t>(*control), Condition::Held};
  }
  const JsonValue* when = MemberOf(_node, "when");
  if (_chordPart)
  {
    if (when != nullptr)
    {
      return Refuse(_path, "a chord's part has no condition");
    }
    return binding;
  }
  if (when == nullptr || when->kind != JsonKind::String)
  {
    return Refuse(_path, "a device leaf needs its condition, when");
  }
  const std::optional<Condition> condition = ConditionOfName(when->text);
  if (!condition.has_value())
  {
    return Refuse(_path, std::format("{} is not a condition", when->text));
  }
  binding.leaf.condition = *condition;
  return binding;
}

Parsed OperandsOf(const JsonValue& _list, InputBinding::Kind _kind, const std::string& _path)
{
  if (_list.kind != JsonKind::Array)
  {
    return Refuse(_path, "the operands are not a list");
  }
  InputBinding binding;
  binding.kind = _kind;
  for (std::size_t index = 0; index < _list.items.size(); ++index)
  {
    Parsed operand = NodeOf(_list.items[index], std::format("{}[{}]", _path, index), _kind == InputBinding::Kind::Chord);
    if (!operand.has_value())
    {
      return operand;
    }
    binding.operands.push_back(std::move(*operand));
  }
  return binding;
}

std::optional<std::uint32_t> WholeSeconds(const JsonValue& _value) noexcept
{
  if (_value.kind != JsonKind::Number || _value.number < 1.0 || _value.number > 4'000'000'000.0 ||
      std::floor(_value.number) != _value.number)
  {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(_value.number);
}

Parsed NodeOf(const JsonValue& _value, const std::string& _path, bool _chordPart)
{
  if (_value.kind != JsonKind::Object)
  {
    return Refuse(_path, "a node is an object");
  }
  std::optional<std::string_view> kind;
  for (const JsonMember& member : _value.members)
  {
    if (std::ranges::find(NODE_KINDS, member.key) == NODE_KINDS.end())
    {
      continue;
    }
    if (kind.has_value())
    {
      return Refuse(_path, std::format("two node-kind properties, {} and {}", *kind, member.key));
    }
    kind = *std::ranges::find(NODE_KINDS, member.key);
  }
  if (!kind.has_value())
  {
    return Refuse(_path, "no node-kind property");
  }
  const auto unknown = [&](std::initializer_list<std::string_view> _allowed) { return UnknownProperty(_value, *kind, _allowed); };
  const std::string inner = std::format("{}.{}", _path, *kind);
  const JsonValue& body = *MemberOf(_value, *kind);
  if (_chordPart && *kind != "key" && *kind != "mouse")
  {
    return Refuse(_path, "a chord's part is a key or a mouse button");
  }
  if (*kind == "pad")
  {
    return Refuse(_path, "pad: the MVP has no gamepad (MVP-100)");
  }
  if (*kind == "key" || *kind == "mouse")
  {
    if (const std::optional<std::string> property = unknown({"when"}))
    {
      return Refuse(_path, std::format("{} is not a property of a device leaf", *property));
    }
    return LeafOf(_value, *kind, _path, _chordPart);
  }
  if (const std::optional<std::string> property = unknown(*kind == "idle"      ? std::initializer_list<std::string_view>{"seconds"}
                                                          : *kind == "setting" ? std::initializer_list<std::string_view>{"negate"}
                                                          : *kind == "value"   ? std::initializer_list<std::string_view>{"flag"}
                                                                               : std::initializer_list<std::string_view>{}))
  {
    return Refuse(_path, std::format("{} is not a property of {}", *property, *kind));
  }
  if (*kind == "all")
  {
    return OperandsOf(body, InputBinding::Kind::All, inner);
  }
  if (*kind == "chord")
  {
    return OperandsOf(body, InputBinding::Kind::Chord, inner);
  }
  if (*kind == "not")
  {
    Parsed operand = NodeOf(body, inner, false);
    return operand.has_value() ? Parsed(NotOf(std::move(*operand))) : operand;
  }
  if (*kind == "alias")
  {
    return body.kind == JsonKind::String ? Parsed(AliasOf(body.text)) : Refuse(_path, "alias names an action id");
  }
  if (*kind == "latch")
  {
    const JsonValue* on = body.kind == JsonKind::Object ? MemberOf(body, "on") : nullptr;
    const JsonValue* off = body.kind == JsonKind::Object ? MemberOf(body, "off") : nullptr;
    if (on == nullptr || off == nullptr || body.members.size() != 2)
    {
      return Refuse(_path, "a latch is an object of exactly on and off");
    }
    Parsed onNode = NodeOf(*on, inner + ".on", false);
    if (!onNode.has_value())
    {
      return onNode;
    }
    Parsed offNode = NodeOf(*off, inner + ".off", false);
    if (!offNode.has_value())
    {
      return offNode;
    }
    return LatchOf(std::move(*onNode), std::move(*offNode));
  }
  if (*kind == "idle")
  {
    const JsonValue* seconds = MemberOf(_value, "seconds");
    if (body.kind == JsonKind::String && body.text == "no_longer")
    {
      return seconds == nullptr ? Parsed(IdleOf(IdleTest::NoLonger)) : Refuse(_path, "idle no_longer takes no seconds");
    }
    if (body.kind != JsonKind::String || (body.text != "at_least" && body.text != "reaches"))
    {
      return Refuse(_path, std::format("{} is not an idle test", body.kind == JsonKind::String ? body.text : std::string("idle")));
    }
    const std::optional<std::uint32_t> whole = seconds != nullptr ? WholeSeconds(*seconds) : std::nullopt;
    if (!whole.has_value())
    {
      return Refuse(_path, "an idle test needs seconds, a whole number of at least 1");
    }
    return IdleOf(body.text == "at_least" ? IdleTest::AtLeast : IdleTest::Reaches, *whole);
  }
  if (*kind == "setting")
  {
    const JsonValue* negate = MemberOf(_value, "negate");
    if (body.kind != JsonKind::String || (negate != nullptr && negate->kind != JsonKind::Boolean))
    {
      return Refuse(_path, "a setting names a setting key, with negate true or false");
    }
    return SettingOf(body.text, negate != nullptr && negate->boolean);
  }
  // value
  const JsonValue* flag = MemberOf(_value, "flag");
  if (body.kind == JsonKind::String && body.text == "always_true" && flag == nullptr)
  {
    return AlwaysTrue();
  }
  if (body.kind == JsonKind::String && body.text == "feature" && flag != nullptr && flag->kind == JsonKind::String)
  {
    InputBinding node = AlwaysTrue();
    node.name = flag->text;
    return node;
  }
  return Refuse(_path, "a value is always_true, or feature with its flag");
}

void AppendQuoted(std::string& _out, std::string_view _text)
{
  _out += '"';
  for (const char c : _text)
  {
    if (c == '"' || c == '\\')
    {
      _out += '\\';
    }
    _out += c;
  }
  _out += '"';
}

void AppendNode(std::string& _out, const InputBinding& _node, bool _chordPart)
{
  switch (_node.kind)
  {
  case InputBinding::Kind::Leaf:
  {
    const bool key = _node.leaf.device == InputDevice::Key;
    _out += key ? "{\"key\": " : "{\"mouse\": ";
    AppendQuoted(_out, key ? NameOfKey(static_cast<KeyCode>(_node.leaf.control))
                           : NameOfMouseControl(static_cast<MouseControl>(_node.leaf.control)));
    if (!_chordPart)
    {
      _out += ", \"when\": ";
      AppendQuoted(_out, NameOfCondition(_node.leaf.condition));
    }
    _out += '}';
    return;
  }
  case InputBinding::Kind::All:
  case InputBinding::Kind::Chord:
  {
    const bool chord = _node.kind == InputBinding::Kind::Chord;
    _out += chord ? "{\"chord\": [" : "{\"all\": [";
    for (std::size_t index = 0; index < _node.operands.size(); ++index)
    {
      _out += index > 0 ? ", " : "";
      AppendNode(_out, _node.operands[index], chord);
    }
    _out += "]}";
    return;
  }
  case InputBinding::Kind::Not:
    _out += "{\"not\": ";
    if (!_node.operands.empty())
    {
      AppendNode(_out, _node.operands[0], false);
    }
    _out += '}';
    return;
  case InputBinding::Kind::Alias:
    _out += "{\"alias\": ";
    AppendQuoted(_out, _node.name);
    _out += '}';
    return;
  case InputBinding::Kind::Latch:
    _out += "{\"latch\": {\"on\": ";
    if (_node.operands.size() == 2)
    {
      AppendNode(_out, _node.operands[0], false);
      _out += ", \"off\": ";
      AppendNode(_out, _node.operands[1], false);
    }
    _out += "}}";
    return;
  case InputBinding::Kind::Idle:
    if (_node.idle == IdleTest::NoLonger)
    {
      _out += "{\"idle\": \"no_longer\"}";
      return;
    }
    _out += std::format("{{\"idle\": \"{}\", \"seconds\": {}}}", _node.idle == IdleTest::AtLeast ? "at_least" : "reaches", _node.seconds);
    return;
  case InputBinding::Kind::Setting:
    _out += "{\"setting\": ";
    AppendQuoted(_out, _node.name);
    _out += _node.negate ? ", \"negate\": true}" : "}";
    return;
  case InputBinding::Kind::Value:
    if (_node.name.empty())
    {
      _out += "{\"value\": \"always_true\"}";
      return;
    }
    _out += "{\"value\": \"feature\", \"flag\": ";
    AppendQuoted(_out, _node.name);
    _out += '}';
    return;
  }
}

} // namespace

std::expected<InputBinding, std::string> ParseBinding(std::string_view _json)
{
  const std::expected<JsonDocument, ContentDiagnostic> document = AuthoringParser{}.Parse(_json, "binding");
  if (!document.has_value())
  {
    return std::unexpected(document.error().message);
  }
  return NodeOf(document->root, "$", false);
}

std::string WriteBinding(const InputBinding& _binding)
{
  std::string out;
  AppendNode(out, _binding, false);
  return out;
}

} // namespace Neuron
