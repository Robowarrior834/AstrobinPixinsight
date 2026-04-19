# Future Revision Notes

Based on a code quality assessment of v2.0.3. The overall architecture is sound and a full rewrite is not recommended — these are targeted improvements in priority order.

---

## What's Working Well (Keep These)

- **Pipeline pattern** — each processing stage as an independent step class is clean and extensible
- **SessionState dataclass** — passing shared state through the pipeline is the right approach
- **Parallel file extraction** — `ProcessPoolExecutor` in `engine/extractor.py` is a good choice
- **Pandas vectorization** — bulk operations are efficient throughout
- **Documentation** — docstrings and module-level docs are comprehensive

---

## High Priority

### 1. Silent Exception Handlers
Several locations swallow exceptions without logging, making user-reported bugs very hard to diagnose.

Locations:
- `AstroBinUpload.py:278` — bare `except: pass` in emergency data dump
- `engine/extractor.py:212, 223, 239` — `except Exception: pass` in XISF parsing
- `engine/steps/geocode.py:175, 205` — bare exception handling in coordinate logic

Fix: Replace with specific exception types and at minimum a `logger.debug()` call so failures leave a trace.

### 2. No Test Suite
The biggest practical risk. There are no pytest tests, so regressions from future changes are invisible until a user reports them.

Suggested coverage:
- Unit tests for each pipeline step using a small synthetic DataFrame
- A test for the XISF header parser against a known fixture file
- A test for the GPS clustering logic with edge-case coordinates
- An integration test that runs the full pipeline against a small set of sample FITS files

### 3. Input Validation
`AstroBinUpload.py:188` expands user-supplied paths but does not verify they exist or are readable before passing them into the pipeline. A clear error message at the entry point is better than a cryptic pandas crash three steps later.

---

## Medium Priority

### 4. Magic Numbers
Undocumented thresholds scattered through the code. They should be named constants with a comment explaining their origin.

| Location | Value | Meaning |
|---|---|---|
| `engine/steps/aggregate.py:57` | `hours=5` | Gap between observations that signals a new session |
| `engine/steps/geocode.py:65` | `0.001` | GPS cluster radius in degrees (~110m at equator) |
| `engine/steps/calibration.py:39` | `0.0001` | Tolerance for EGAIN vs GAIN handshake |

### 5. GPS Clustering Algorithm
`engine/steps/geocode.py:62-85` uses a greedy Euclidean distance approach. Two issues:

- **Order-dependent**: which point becomes the cluster centre depends on DataFrame row order
- **Inaccurate at high latitudes**: Euclidean approximation breaks down; Haversine distance should be used

Replacement options: DBSCAN with Haversine metric (scikit-learn has this built in), or simply use `geopy.distance.distance()` which is already a dependency.

### 6. Centralise Regex Patterns
Filename-parsing regexes are duplicated between `engine/extractor.py` and `engine/steps/base.py`. Move them to `constants.py` under a `RegexPatterns` class so any future change is made in one place.

### 7. Dead Code
`geopy` is listed in `requirements.txt` and imported but the `Nominatim` geocoder is never actually called in the current code. Verify and remove the dead import.

### 8. Boolean Parsing in Config Loader
`engine/loader.py:66` only accepts lowercase 'true' for the USEOBSDATE flag. Should also accept '1' and 'yes' to match normal user expectations.

---

## Low Priority

### 9. Logging Infrastructure
`AstroBinUpload.py:100-131` contains a FunctionNameFilter that walks the call stack via `inspect.stack()` on every log record. This is slow and fragile. Standard %(funcName)s in the log format string achieves the same result without the overhead.

### 10. Reports Module Coupling
`engine/reports.py` mixes data preparation with string formatting in the same functions. If a second output format (JSON, HTML) is ever needed, these would need to be separated.

### 11. ConfigObj Dependency
`configobj` is less actively maintained than Python's built-in `configparser`. A future revision could migrate to `configparser` or to TOML, which has built-in support from Python 3.11+.

---

## Where to Start If Doing a Full Revision

1. Write the test suite first against the existing code — this gives you a safety net
2. Fix silent exception handlers — low effort, high diagnostic value
3. Add input validation at `AstroBinUpload.py` entry point
4. Replace the GPS clustering with a Haversine-based approach
5. Centralise constants and regex patterns
6. Tackle the reporting module separation last, as it carries the most rewrite risk

---

## Files of Interest for a Revision

| File | Notes |
|---|---|
| `engine/extractor.py` | XISF parsing has the most silent failures |
| `engine/steps/geocode.py` | GPS clustering needs the most algorithmic improvement |
| `engine/steps/base.py` | Largest single step; candidate for splitting |
| `engine/reports.py` | Tightly coupled; good candidate for a data/presentation split |
| `constants.py` | Already well-structured; add `RegexPatterns` class here |

