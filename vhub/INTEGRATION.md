# INTEGRATION — for product firmware repos

This guide is for **firmware maintainers**. It explains how to author and
maintain the verification template (`tests.json`) that ships with each
product release and is consumed by VHUB.

> TL;DR: drop a `verification/tests.json` next to your firmware source, edit
> it as you add/remove test scenarios, validate it locally with
> `validate_template.py`, and upload it into VHUB through the web UI for each
> release.

---

## 1. What is a "template"?

A template is an **immutable list of test cases** for one product. It lives
in the firmware repo because:

- Adding a feature or sensor often adds/removes test cases — they belong
  next to the code that defines them.
- Each release ships with the template that describes how it should be
  tested, so QA always validates against the right list.

VHUB stores each imported template as a frozen snapshot. Future imports do
not mutate existing releases.

---

## 2. File location & format

**Convention:** `verification/tests.json` at the firmware repo root.

```json
{
  "schema_version": "1.0",
  "rev": "a1b2c3d",
  "product": {
    "slug": "one-indoor",
    "name": "ONE Indoor",
    "variants": ["ONE_INDOOR", "I-9PSL"]
  },
  "tests": [
    {
      "id": "boot.ledbar.short-press-no-wifi",
      "category": "Boot",
      "sub_category": "LED BAR",
      "description": "If LED bar test requested with short press, WiFi connector should not perform",
      "applies_to": ["ONE_INDOOR"],
      "expected_result": "WiFi connector skipped",
      "notes": "Pressing button while display shows 'press now for led test' also activates offline mode"
    }
  ]
}
```

### Field reference

| Field                 | Required | Notes                                                        |
| --------------------- | -------- | ------------------------------------------------------------ |
| `schema_version`      | yes      | Must be `"1.0"`. Do NOT bump per import — only when VHUB itself ships a new schema. |
| `rev`                 | yes      | Client-supplied snapshot identifier. **Unique per product.** See §2.1. |
| `product.slug`        | yes      | Must match the slug of the VHUB product. `[a-z0-9-]+`.       |
| `product.name`        | yes      | Display name.                                                |
| `product.variants`    | yes      | At least 1 unique entry. Used as columns in the test matrix. |
| `tests[].id`          | yes      | Stable string. `[a-z0-9][a-z0-9._-]*`. Unique within file.   |
| `tests[].description` | yes      | What is being verified.                                      |
| `tests[].applies_to`  | yes      | Subset of `product.variants`. At least 1 entry.              |
| `tests[].category`    | no       | Top-level grouping (`Boot`, `Common Flow`, ...).             |
| `tests[].sub_category`| no       | Second-level grouping.                                       |
| `tests[].expected_result` | no   | What "pass" looks like for the tester.                       |
| `tests[].notes`       | no       | Edge cases, gotchas, environmental prerequisites.            |

### 2.1 The `rev` field

`rev` uniquely identifies *this snapshot of the template* and is what VHUB
uses to deduplicate imports. Re-importing the same `(product, rev)` pair is
rejected with a clear error.

- Pattern: `^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$` (1..64 chars).
- Recommended source: **short git commit hash** of the firmware repo at the
  time the template was authored (e.g. `git rev-parse --short HEAD`).
- Any other unique string works (release tag, ISO date, build hash, UUID).
- Bump `rev` every time the template content changes — that's the whole
  point of the field.

A useful pattern is to derive `rev` from the same commit that ships the
firmware artifact, so the firmware version, binary, and template are all
traceable to one source point.

### Identity rules

- **Test `id` is the identity** — pick something descriptive and *stable*.
  Reordering tests does not break anything; renaming an `id` does.
- A test can apply to one or many variants. Anything not in `applies_to` is
  rendered as `n/a` in the VHUB matrix for that variant.
- New variants are added to `product.variants` and referenced in
  `applies_to` of the relevant tests.

---

## 3. Local validation

VHUB ships a standalone CLI you can vendor into your firmware repo (or call
remotely from your CI). It has **no runtime dependency on VHUB itself**.

### Option A — install from vhub repo

```bash
git clone <vhub-repo>
pip install jsonschema      # only dependency
python vhub/tools/validate_template.py path/to/tests.json
```

### Option B — copy the script + the schema

Copy two files into your firmware repo:

- `tools/validate_template.py`
- `schema/template.v1.json`

Set `VHUB_SCHEMA_PATH` so the script knows where to look:

```bash
VHUB_SCHEMA_PATH=schema/template.v1.json \
  python tools/validate_template.py verification/tests.json
```

### Exit codes

| Code | Meaning                                                          |
| ---- | ---------------------------------------------------------------- |
| `0`  | OK — template is valid and ready to import.                      |
| `1`  | One or more validation errors (printed to stderr).               |
| `2`  | File not found, unreadable, or invalid JSON.                     |

Validation rules (full set):

- JSON structure matches the schema (`schema/template.v1.json`).
- `schema_version` is supported.
- `product.slug` matches the regex.
- Test `id`s match the regex **and are unique** within the file.
- Every `applies_to` entry exists in `product.variants`.
- `tests[]` is non-empty.

---

## 4. Recommended CI hook

Run the validator on every PR that touches `verification/`:

```yaml
# .github/workflows/validate-tests.yml (example)
name: Validate verification template
on:
  pull_request:
    paths: [verification/**]
jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with: { python-version: '3.12' }
      - run: pip install jsonschema
      - run: python tools/validate_template.py verification/tests.json
```

---

## 5. Release-time workflow

```
┌─────────────────────────────────────────────────────────────────┐
│ Firmware repo (per product)                                     │
│                                                                 │
│  1. Edit verification/tests.json alongside the code change.     │
│  2. CI runs validate_template.py — must be green.               │
│  3. Tag firmware release. tests.json is part of the artifact.   │
└─────────────────────────────────────────────────────────────────┘
                          │
                          │  manual upload (v1)
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│ VHUB                                                            │
│                                                                 │
│  4. QA opens Product → Import Template → uploads tests.json.    │
│  5. QA opens Product → New Release → enters version (`3.1.8`).  │
│  6. Tester fills the matrix. Mark Completed when done.          │
│  7. Export CSV for stakeholders / archival.                     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6. Multi-product codebases

If one firmware codebase serves several products (e.g. ONE & Open Air share
~80% of tests), emit **one `tests.json` per product** at build time. Keep a
single internal source-of-truth that filters to per-product variants —
that's an implementation detail of your build, not of VHUB. VHUB treats each
product as an independent database.

Example layout in firmware repo:

```
verification/
├── shared/            # shared scenarios (not consumed by VHUB directly)
├── build_templates.py # script that emits per-product JSON
└── dist/
    ├── one-indoor.tests.json
    └── open-air.tests.json
```

`build_templates.py` is your responsibility. VHUB only consumes the final
per-product files.

---

## 7. Common pitfalls

- **Slug mismatch** — `product.slug` inside `tests.json` must equal the slug
  of the target product in VHUB. The importer rejects mismatches to prevent
  cross-product imports.
- **Renaming `id`** — VHUB tracks results by template snapshot + test_case
  id. Once a release is created against a template, that mapping is frozen.
  Renaming an id in a *new* import creates a different test case; old
  releases are unaffected.
- **Removing a variant** — fine for *new* templates; old releases keep their
  original variant set.
- **Empty `applies_to`** — rejected. If a test doesn't apply to anything,
  delete it.

---

## 8. Schema evolution

`schema_version` is required and currently fixed at `"1.0"`. When VHUB
introduces breaking changes the new version will:

- Be added to `SUPPORTED_SCHEMA_VERSIONS` in the importer.
- Get its own schema file under `schema/template.vN.json`.
- Include a migration note in this document.
