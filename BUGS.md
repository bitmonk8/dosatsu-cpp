# Known Issues in MakeIndex

> **DOCUMENT PRINCIPLES:**
> - Issues present in this document are **NOT YET FIXED**
> - Issues absent from this document are either **FIXED** or **NOT YET DISCOVERED**
> - No status indicators are used - presence in document = unfixed
> - No numbering is used to avoid maintenance overhead

## Current Issues

### Test Files Don't Match Expected Content
- **Issue**: Tests look for classes like "Animal", "Mammal" etc. but current test files have different classes
- **Root Cause**: Test files were simplified to avoid standard library includes
- **Impact**: Content-specific tests fail even with correct queries
- **Fix Needed**: Either update tests to match current content or create richer test files without std library




