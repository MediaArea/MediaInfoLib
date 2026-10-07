# Count localization

The numeric-unit formatter and GUI count messages use plural rules supplied
by the selected catalog. Rules are compiled when the language is loaded.
Existing language rules remain as a fallback for catalogs without valid metadata.
The C++98 implementation uses no imported rule data or additional dependencies.
Only human-readable output changes; raw fields and JSON/XML schemas are unchanged.

## Catalog rule metadata

MediaInfo catalogs define rules in the metadata rows below. For example, English:

```text
  Config_Text_PluralRules;1
  Config_Text_PluralRule.one;v == 0 && i == 1
  Config_Text_PluralLegacy;one=1,other=2
  Config_Text_PluralLegacyDecimals;1
```

The version must be `1`. Optional `Config_Text_PluralRule.<category>` entries
use `zero`, `one`, `two`, `few` and `many`, evaluated in that order; the first
match wins. `other` is implicit. Empty or absent expressions disable a category.
A versioned catalog with no predicates therefore always selects `other`.
All metadata keys retain the two leading spaces used by other catalog settings.

Expressions support operands `i` (integer digits), `v` (visible fractional digit
count) and `f` (fractional digits interpreted as an integer, or zero if absent).
For `1.20`, these are `1`, `2` and `20`. Signs are ignored. Counts are kept as
exact digit strings, including values beyond fixed-width integer limits.
An operand may be followed by `%` and a positive integer constant. Comparisons
use `==`, `!=`, `<`, `<=`, `>` or `>=` followed by a nonnegative integer constant.
Combine comparisons with `&&`, `||` and parentheses; `&&` binds more tightly
than `||`. Boolean literals `true` and `false` are also accepted. There are no
other arithmetic operators or function calls. Constants and moduli are at most
1,000,000; expressions are limited to 1,024 bytes and 16 nested parentheses.
Every branch is validated, including branches whose result is already determined.

`Config_Text_PluralLegacy` maps categories to existing numbered catalog suffixes,
for example `one=1,few=2,other=3`. It is optional; if present, `other` is required
and supplies the mapping for unspecified categories. Values are `1`, `2`, `3`,
or `0` to disable that legacy form. Duplicate categories are invalid. Without
this mapping, only named messages and invariant units use the catalog rules.
`Config_Text_PluralLegacyDecimals` is `1` if numbered legacy forms support
fractional notation, otherwise `0` (the default). Named messages may always
express fractional categories. Existing exact-zero messages remain supported.

Valid metadata takes precedence over the language identifier, allowing new
languages without a C++ rule change. Unsupported versions, malformed expressions
or invalid legacy settings discard the entire metadata policy and use the
existing language fallback. Rules are read from the original selected catalog;
missing metadata never inherits English rules from the merged translations.

In MediaInfo, keep these rows in both `Source/Resource/Language.csv` and the
individual `Source/Resource/Plugin/Language/*.csv` catalogs. The PreRelease
workflow merges individual catalogs with `Language_Others_Run` before splitting
them with `Language_All_Run`. Do not run the split step on a metadata-only matrix.

## Public GUI interface

`MediaInfo::Option_Static("Language_Format")` returns `"1"` when supported.
Pass a `ZtringListList` serialized request as the second argument:

```text
Message;StreamSummary
Count;6
Kind;Audio
Formats;AAC
```

Messages are `FileCount` (Message/Count), `StreamCount` and `StreamSummaryMore`
(also Kind), and `StreamSummary` (also Formats). Kinds are Audio, Video, Text,
Image, Other and Menu. Counts are unsigned decimal integers. Unknown, duplicate,
missing or invalid arguments return an empty result; GUI clients provide a
complete English fallback. Serialize metadata with `ZtringListList` so quotes,
semicolons and line breaks remain literal. No private library header is needed.

Catalog keys are `FileCount.<category>` or `<message>.<kind>.<category>`, with
categories zero/one/two/few/many/other. `other` is required. Patterns contain
exactly one `{count}` and, for StreamSummary, exactly one `{formats}`. `{{`/`}}`
escape braces. Inserted values are not reinterpreted. Invalid patterns fall back
to legacy entries, then a complete English message. Language changes replace the
unmerged catalog atomically; there is no cached pattern state.

## Compatibility

Without valid rule metadata, legacy suffixes are interpreted per catalog: cs/sk/pl/ru/be/uk/hr/lt/ro use
one=1, few=2, remaining integer categories=3; other existing catalogs use
one=1, remaining categories=2. Existing invariant units and exact `0` whole
messages retain their meanings. Missing local forms are resolved from the
English family using English rules. Historical identifiers gr and pt mean el
and pt-PT; pt-BR retains Brazilian rules. Unknown locales fall back to English
unless the unit is invariant. New catalog languages can supply rule metadata
and named messages. Existing Arabic and other incomplete catalog triplets
remain approximations; named messages can express every category.

Numeric units accept `[+-]?[0-9]+(\.[0-9]+)?`. Signs are ignored for plural
selection; visible fraction digits and arbitrarily large counts are preserved.
Named unit messages (` channel.<category>`) support fractional categories
and require a ` channel.other` fallback. Legacy cs/sk/pl/ru/be/uk/hr
triplets have no fractional form and fall back to English unless invariant.
Expressions such as `29.97 (30000/1001)` remain literal and may only receive an
invariant unit. Other invalid inputs are returned unchanged. Number grouping is
outside this change and retains its existing presentation policy.
Literal measurement symbols absent from catalogs (such as TiB and cd/m2) retain
the existing numeric-unit fallback; this does not apply to GUI message keys.

## Regression tests

Configure `Project/CMake` with `BUILD_SHARED_LIBS=OFF` and
`MEDIAINFO_BUILD_TESTS=ON`, build `count_localization`, then run
`ctest -R '^count_localization$' --output-on-failure`. Set
`MEDIAINFO_LANGUAGE_DIR` if the MediaInfo checkout is not adjacent.
Tests use actual catalogs, the public option, production numeric formatting,
and a parsed 22-channel WAV. Direct selector regressions cover the supported
language families, decimal notation, large counts, locale variants and invalid input.
Metadata tests cover parser validation, precedence, legacy mappings, language
changes and actual catalogs with their language identifiers removed from the
compatibility lookup. This ensures that metadata, rather than the fallback,
drives the observed categories.
