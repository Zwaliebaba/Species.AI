#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Neuron
{

// ast.pack.max_key_bytes: the longest key (AST-014, DEC-801).
inline constexpr std::size_t MAX_KEY_BYTES = 255;

// The content type of a cooked entry (AST §2.1). Its position is the content-type byte of a pack entry and of a
// loose-content index entry, so values are only appended, and a reader refuses one it does not know (AST-128).
enum class ContentType : std::uint8_t
{
  Texture,
  CpuTable,
  Mesh,
  Collision,
  SimRecord,
  Record,
  Map,
  MapIndex,
  StringTable,
  Font,
  ResidentSound,
  SoundStream,
  ExplainerScript,
  AssetIndex
};

inline constexpr std::size_t CONTENT_TYPE_COUNT = 14;

// The content type's name, as the build report and the log print it.
[[nodiscard]] std::string_view ContentTypeName(ContentType _type) noexcept;

// AST-014: a key is 1 to MAX_KEY_BYTES bytes of lower-case ASCII letters, digits and `_`, `.`, `/` and `@`, with no
// empty segment between two `/`.
[[nodiscard]] bool IsCanonicalKey(std::string_view _key) noexcept;

// AST-015: a requested key, canonicalised before the look-up: ASCII upper-case letters become lower case and `\`
// becomes `/`. The result is looked up exactly.
[[nodiscard]] std::string CanonicalKey(std::string_view _requested);

// AST-014: the key of a source, from its path relative to the content root with forward slashes: its extension
// removed and its letters lower-cased (O61). "Textures/Sprite/WorkerFront.dds" gives "textures/sprite/workerfront".
[[nodiscard]] std::string KeyOfSource(std::string_view _relativePath);

} // namespace Neuron
