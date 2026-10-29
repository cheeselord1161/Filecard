# Filecard

Filecard creates a quick, local overview of a project directory and writes it as an HTML report. It opens a native folder chooser on launch; if that dialog is canceled, you can type a path instead.

## What it reports

- Regular file and subfolder counts, including nested folders
- Counts for selected file extensions (`.cpp`, `.h`, `.py`, `.js`, `.ts`, `.go`, `.rs`, `.java`, `.html`, `.css`, and `.php`)
- Common top-level license files, with heuristic type labels for MIT, Apache-2.0, GNU GPL/AGPL/LGPL, BSD, ISC, MPL, Boost, Unlicense, and CC0
- README, build, and project marker files at the selected directory's top level
- A standalone HTML report that can be opened in a browser

Filecard skips common generated or dependency directories such as `.git`, `.svn`, `.hg`, `node_modules`, `vendor`, `build`, `dist`, `out`, `target`, `bin`, `obj`, `debug`, and `release`.

The scan is read-only. File extension counts are indicators, not definitive language detection. Filecard recognizes common license text patterns and SPDX identifiers; modified or uncommon licenses may be reported as unknown. It does not parse dependency manifests.

## Build

Filecard requires a C++17 compiler with `std::filesystem` support.

With GCC or MinGW, compile Filecard together with the included folder-dialog source:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp \
  third_party/tinyfiledialogs/tinyfiledialogs.cpp -o filecard
```

On Windows with MinGW, add the dialog library's required Windows libraries:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp \
  third_party/tinyfiledialogs/tinyfiledialogs.cpp \
  -lcomdlg32 -lole32 -o filecard.exe
```

On Linux, the folder chooser uses an available desktop dialog helper such as Zenity or KDialog. Install one if the chooser cannot open.

The included `Filecard.cbp` project is for Code::Blocks. Open it there and build the Debug or Release target. The project sets the C++17 compiler option and includes the dialog source. On Windows/MinGW, add `-lcomdlg32 -lole32` to the target's linker options.

## Use

Run the program and select the directory in the folder dialog. If you cancel the dialog, Filecard offers a text-path fallback. At the report destination prompt, press Enter to save `filecard-report.html` inside the selected directory. You may enter a filename, a full file path, or a directory; if you enter a directory, Filecard places `filecard-report.html` inside it. Relative paths are resolved inside the selected project directory. Filecard prints the exact destination before asking whether to replace an existing report.

Paths containing spaces are accepted because Filecard reads the full input line.

## Third-party component

The cross-platform folder dialog is [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) v3.22.1 by Guillaume Vareille. It is distributed under the zlib license; see `third_party/tinyfiledialogs/` for its source and license notice.

## Known limitations

- Filecard is a console application with a native folder-selection dialog; report output is still entered as a path.
- Some inaccessible entries may be skipped. The program reports scan issues when the filesystem provides an error.
- An existing report is replaced only after the user confirms the overwrite prompt.
- Language detection is based on a short list of extensions and does not inspect source contents.
- Dependency detection and dependency parsing are not implemented yet.

## Contributing

Bug reports and improvements are welcome. Please include the operating system, compiler, steps to reproduce, and the relevant error output when reporting a problem.

## License

Filecard is distributed under the MIT License. See [LICENSE](LICENSE).
