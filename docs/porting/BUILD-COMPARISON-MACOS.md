# Build the file-comparison app on Apple Silicon

Use a native macOS Terminal (not Rosetta). M1 through M5 use the arm64 build.
Install Apple's command line tools with `xcode-select --install` if missing, and
Homebrew from https://brew.sh if missing. Complete these installations before proceeding.

Clone the feature branch into a new directory:

```sh
git clone --branch feature/file-comparison --single-branch https://github.com/GioBoSoft/FreeSCP.git FreeSCP-comparison
cd FreeSCP-comparison
bash native/scripts/build-macos-comparison.sh
```

The script installs Qt and build dependencies with Homebrew, uses a dedicated
`native/build-comparison-arm64` directory, builds the complete app and runs CTest.
Debug mode keeps assertions enabled in the regression tests. If tests pass, it bundles
Qt, applies an ad-hoc signature and creates a ZIP in a unique package directory.
Finder opens with FreeSCP.app selected. No Apple Developer account is required;
the app is not notarized. Build output is saved in
`native/build-comparison-arm64/build.log`; CTest details are in
`native/build-comparison-arm64/Testing/Temporary/LastTest.log`.

GitHub Actions now uses the standard macos-26 ARM64 runner to try another image
following macos-15 capacity failures. This does not guarantee immediate runner allocation.
The Actions artifact is named FreeSCP-macOS-arm64 and is retained for 14 days.

Validation: the local script passes bash syntax validation. macOS execution and full
application build remain pending; this script cannot be executed on the Linux workspace.
