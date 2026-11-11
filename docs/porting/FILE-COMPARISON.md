# Local/remote file comparison

Select one regular file in each panel, then choose **Commands → Compare files…**.
The remote file is downloaded to a unique, automatically removed temporary directory.
The comparison never uploads or modifies either original file.

Text is shown side by side, with original line numbers, aligned gaps, highlighted
changed rows, synchronized vertical scrolling, and previous/next navigation.
UTF-8 and BOM-detected encodings are decoded using Qt. Binary or invalidly encoded
files receive a byte-equality result instead. Whitespace and line endings are significant.
The POSIX `diff` executable must be available on PATH; it is invoked directly without a shell.

Current limits: 8 MiB per file; diff timeout of 10 seconds. Download and comparison
are synchronous, following the existing internal editor workflow, so the interface
may pause. Changes within individual lines, editing/merge, alternate legacy encodings,
and asynchronous cancellation are not implemented. The comparison represents a downloaded
snapshot of the remote file, not a live view.

Validation: standalone Qt 6.8 component compiled and linked on Linux; six offscreen
regression cases cover insertion, deletion, replacement, empty inputs, and final-newline
differences. The integrated main.cpp passed a compiler syntax check. Full application
linking, live-server transfers, and macOS execution have not been verified.

When BUILD_TESTING is enabled, CMake adds the filecompare-test executable and the
filecompare CTest entry. The tests do not require credentials or a server.
