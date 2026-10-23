---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T15, owner decision O94)
---

# The rest of the input router at M2

## Context and Problem Statement

M1 built the input router's subset with the bindings compiled in ([ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md), O75). Those bindings were device leaves, `all` of leaves, and chords, with no layers. M2's T14 added the repeat condition ([ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md)).

T15 asks for the rest of ADR-029's contract, now that the settings store exists ([ADR-150](ADR-150-the-settings-store-and-user-data-root-at-m2.md)):
- the name table, with its parser and writer;
- the layout and user layers;
- latch and the other binding operators;
- the input mode and idle time.

Its run checklist asks that a rebinding written to the user layer take effect without a rebuild. O94 settles where the user layer lives: `binding.<action id>` lines in `settings.txt`, read when the client starts.

## Decision Drivers

* INP-162: the layers in order, default, development, layout, user, each later one replacing primary bindings.
* INP-072 and INP-073: a binding written and read back is the same binding, and a node that breaks the notation is refused, never defaulted (DEC-096).
* INP-171: a bad user entry never stops the client and never changes the action.
* MVP-100: the MVP has no gamepad.

## Considered Options

1. **Bindings as a tree of nodes in the router, with the default set compiled in and the replacing layers given to `Rebuild`**.
2. **The default set and layout layers as `input.binding_set` records** (INP-070, DAT-202), with a schema, a cooker and the content build's validation.
3. **The user layer re-read whenever `settings.txt` changes**.

## Decision Outcome

Chosen option: 1.
- Option 2 adds a schema, a cooker and a reader for one default set and two layout layers that no one edits but in code. O75's compiled-in set stands until a binding is tuned without a rebuild.
- Option 3 is O94's rejected alternative.

* **The binding node** (`InputBinding`, INP-060) is a tree:
  - **Kinds.** Device leaf, `all`, `chord`, `not`, `alias`, `latch`, `idle`, `setting` and `value`, with helpers for each.
  - **Equality.** Two bindings are equal when every node matches, so a binding read back from its notation compares equal to the one written.
  - **Run-time state.** A latch's state and a setting's value live in the router's state table at the node's slot, which `Rebuild` assigns (INP-066). An alias's slot is its target action.
* **The checks** (`InputRouter::CheckBinding`, INP-060, INP-061, INP-172) refuse, with a message naming the word:
  - a condition the control does not accept (table 3.2-D);
  - an `all` with fewer than two operands, an `all` in an `all`, or more than one analogue operand;
  - a chord part that is not a key or a mouse button;
  - a `not` of a non-Boolean operand, or of an `all` or a chord;
  - an alias of no action, or an alias chain back to its own action (DEC-554);
  - a latch in a latch, or a non-Boolean latch operand;
  - an idle test without whole seconds;
  - a setting the store does not hold;
  - any feature flag, since no edition flag record exists at M2 (INP-143);
  - a Boolean binding on an analogue action (INP-061).

  The compiled-in set passes the same checks when the router is made.
* **The name table** (`NeuronClient/InputNames`, INP-055, INP-056):
  - table 3.2-A's 113 key names;
  - table 3.2-B's six mouse controls;
  - INP-051's nine conditions.

  The table gives one name per control and back, matched without regard to ASCII case. The gamepad's analogue conditions have names, and no control of the MVP accepts them.
* **The notation** (`NeuronClient/BindingNotation`):
  - **Parsing.** `ParseBinding` reads one JSON node through the strict authoring parser and refuses anything INP-073 refuses: an unknown property, no or two node-kind properties, a missing condition or seconds, an unknown name, and a chord part with a condition. A gamepad leaf is refused under MVP-100. The message names the node's path, such as `$.all[1]`.
  - **Writing.** `WriteBinding` writes the canonical form of INP-072 on one line, in INP-071's property order, with spaces after colons and commas as the table shows.
* **The layers** (INP-162 to INP-166):
  - **Rebuild.** `InputRouter::Rebuild` makes the bindings again from the default set and applies each replacement in order as its action's primary binding (INP-065). A replacement naming no action, or one the checks refuse, is skipped with a warning naming its source (INP-171). The latches restart false.
  - **The layout layer** (`GameClient/InputBindings`) is table 4.13-B's, compiled in, for `de-DE` and `fr-FR`. The active layout is the `input.keyboard_layout` setting when set, else the window's system layout tag (INP-164), and a system layout change rebuilds the bindings when the active layout changes (INP-166).
  - **The user layer** is every `binding.<action id>` setting parsed with `ParseBinding`. One that does not parse is skipped with a warning naming its key. It is read at start-up and on a layout change (INP-203), so a rebinding takes effect at the next start (O94).
* **The new nodes' behavior:**
  - **`not`, `alias` and `value:always_true`** follow INP-138, INP-139 and INP-143. An alias's query includes its target's suppression.
  - **Latches.** Each is updated once a frame at the end of `AdvanceFrame` as state := (state ∧ ¬off) ∨ (¬state ∧ on) (INP-140).
  - **Idle.** The idle time grows by the real time between frames while no key or button is held or had an event and the pointer and the wheel stayed still, and is 0 in an active frame (INP-151, INP-153). `at_least`, `reaches` and `no_longer` read it (INP-141).
  - **Settings.** A setting node is true when its setting's number is above 0, a boolean being 1. It is read once a frame through the game's `SettingAccess` (INP-142).
* **The input mode** is `KeyboardMouse` (INP-150). With no gamepad back-end, nothing changes it, and no mode listener exists yet.
* **The client** builds the router with the settings' access, rebuilds it from the layout and user layers before its first frame, and logs how many bindings were replaced. The frame bench keeps the default set, so its measurements never depend on a player's settings.
* **What waits:**
  - the event queue's capacity and its reconciliation after an overflow (INP-115 to INP-117);
  - INP-178's descriptions, and with them the binding arguments of strings (ADR-146), which wait for the controls' names in the string table;
  - the rebinding hook's services (INP-175 to INP-179), which no player reaches (DEC-050);
  - the gamepad and its mode (MVP-100);
  - the binding sets as records.

### Consequences

* Good, because a player can rebind any action by one line in `settings.txt`, and a broken line costs that line only.
* Good, because every binding the client holds, compiled in or typed by hand, passes the same checks.
* Good, because the notation reads back exactly, which the rebinding hook and the binding arguments of strings will need.
* Bad, because a rebinding needs a restart (O94), and the layout layers are still code.
* It forecloses: bindings that fall back to a default condition (DEC-096); a user layer outside the settings store.

### Confirmation

* NeuronClientTests:
  - `BindingNotationTests`: every row of INP-071 reads back canonically; names in any case and order; the refusals of INP-073 and MVP-100, with the node's path; and the 113 key names round-trip.
  - `InputRouterTests`:
    - a replacing layer replaces the primary only and keeps the arrow keys, and skips an analogue action's Boolean binding and an unknown action;
    - an alias with its target's suppression;
    - `not` and `value`;
    - an alias cycle refused;
    - a latch toggled and reset by `Rebuild`;
    - the idle time and its three tests;
    - a setting node read once a frame, and its negation.
* GameClientTests: `InputBindingsTests`: table 4.13-B's layers, the keyboard-layout setting winning over the system's, the user layer's parse and order after the layout layer, and the canonical form.
* The owner's T15 run: a line such as `binding.action.select.clear = {"key": "x", "when": "down"}` in `settings.txt` makes X clear the selection after a restart, and a broken line is logged and changes nothing.

## Pros and Cons of the Options

### 1 — a tree of nodes, the default set compiled in

* Good, because every node kind and both replacing layers land with their tests now, without a content format.
* Bad, because a default binding's change is a rebuild.

### 2 — binding sets as records

* Good, because INP-070's form is complete.
* Bad, because it costs a schema, a cooker and a reader before anyone tunes a default binding.

### 3 — the user layer re-read while running

* Good, because a rebinding takes effect at once.
* Bad, because it was not O94's choice, and it needs a watch on the settings file for an edit that a restart serves.

## More Information

* Amends [ADR-029](ADR-029-input-event-model-and-binding-contract.md) and [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md); amends R-META-408 of [options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) (O94).
* The code: `NeuronClient/InputRouter`, `InputNames`, `BindingNotation`; `GameClient/InputBindings`, `ClientSettings`; `Species/Species.cpp`.
