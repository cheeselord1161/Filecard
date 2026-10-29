#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "third_party/tinyfiledialogs/tinyfiledialogs.h"

namespace fs = std::filesystem;

std::string lowercase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return text;
}

std::string htmlEscape(const std::string& text) {
    std::string escaped;
    for (char ch : text) {
        switch (ch) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&#39;"; break;
        default: escaped += ch; break;
        }
    }
    return escaped;
}

bool isLicenseFilename(const std::string& lowerName) {
    return lowerName == "license" || lowerName.rfind("license.", 0) == 0 ||
           lowerName.rfind("license-", 0) == 0 || lowerName == "copying" ||
           lowerName.rfind("copying.", 0) == 0 || lowerName.rfind("copying-", 0) == 0;
}

std::string detectLicenseType(const fs::path& path) {
    std::ifstream licenseStream(path, std::ios::binary);
    if (!licenseStream) {
        return "Could not read license file";
    }

    const std::string text(
        (std::istreambuf_iterator<char>(licenseStream)),
        std::istreambuf_iterator<char>());
    const std::string lowerText = lowercase(text);

    // Prefer SPDX identifiers when the file provides one.
    const std::pair<const char*, const char*> spdxLicenses[] = {
        {"spdx-license-identifier: mit", "MIT"},
        {"spdx-license-identifier: apache-2.0", "Apache-2.0"},
        {"spdx-license-identifier: gpl-3.0", "GPL-3.0"},
        {"spdx-license-identifier: gpl-2.0", "GPL-2.0"},
        {"spdx-license-identifier: agpl-3.0", "AGPL-3.0"},
        {"spdx-license-identifier: lgpl-3.0", "LGPL-3.0"},
        {"spdx-license-identifier: mpl-2.0", "MPL-2.0"},
        {"spdx-license-identifier: bsd-3-clause", "BSD-3-Clause"},
        {"spdx-license-identifier: bsd-2-clause", "BSD-2-Clause"},
        {"spdx-license-identifier: isc", "ISC"},
        {"spdx-license-identifier: unlicense", "Unlicense"},
        {"spdx-license-identifier: bsl-1.0", "BSL-1.0 (Boost)"},
        {"spdx-license-identifier: cc0-1.0", "CC0-1.0"}
    };
    for (const auto& license : spdxLicenses) {
        if (lowerText.find(license.first) != std::string::npos) {
            return license.second;
        }
    }

    if (lowerText.find("gnu affero general public license") != std::string::npos) {
        if (lowerText.find("version 3") != std::string::npos) return "GNU AGPL v3";
        return "GNU AGPL (version not identified)";
    }
    if (lowerText.find("gnu lesser general public license") != std::string::npos ||
        lowerText.find("gnu library general public license") != std::string::npos) {
        if (lowerText.find("version 3") != std::string::npos) return "GNU LGPL v3";
        if (lowerText.find("version 2.1") != std::string::npos) return "GNU LGPL v2.1";
        return "GNU LGPL (version not identified)";
    }
    if (lowerText.find("gnu general public license") != std::string::npos) {
        if (lowerText.find("version 3") != std::string::npos) return "GNU GPL v3";
        if (lowerText.find("version 2") != std::string::npos) return "GNU GPL v2";
        return "GNU GPL (version not identified)";
    }
    if (lowerText.find("apache license") != std::string::npos &&
        lowerText.find("version 2.0") != std::string::npos) {
        return "Apache-2.0";
    }
    if (lowerText.find("mozilla public license") != std::string::npos &&
        lowerText.find("2.0") != std::string::npos) {
        return "MPL-2.0";
    }
    if (lowerText.find("permission is hereby granted, free of charge, to any person obtaining a copy")
        != std::string::npos) {
        return "MIT";
    }
    if (lowerText.find("permission to use, copy, modify, and/or distribute this software")
        != std::string::npos) {
        return "ISC";
    }
    if (lowerText.find("redistribution and use in source and binary forms") != std::string::npos) {
        if (lowerText.find("all advertising materials mentioning features or use") != std::string::npos) {
            return "BSD-4-Clause";
        }
        if (lowerText.find("neither the name") != std::string::npos ||
            lowerText.find("endorse or promote products") != std::string::npos) {
            return "BSD-3-Clause";
        }
        return "BSD-2-Clause";
    }
    if (lowerText.find("this is free and unencumbered software released into the public domain")
        != std::string::npos) {
        return "Unlicense";
    }
    if (lowerText.find("boost software license") != std::string::npos ||
        lowerText.find("permission is hereby granted, free of charge, to any person or organization")
        != std::string::npos) {
        return "BSL-1.0 (Boost)";
    }
    if (lowerText.find("cc0 1.0 universal") != std::string::npos) {
        return "CC0-1.0";
    }

    return "Unknown license text";
}

int main() {
    std::string folderText;
    const char* selectedFolder =
        tinyfd_selectFolderDialog("Choose a project folder", "");

    if (selectedFolder != nullptr) {
        folderText = selectedFolder;
    } else {
        std::cout << "Folder selection was canceled. Enter a path manually (or press Enter to quit): ";
        std::getline(std::cin, folderText);
        if (folderText.empty()) {
            return 0;
        }
    }

    const fs::path folder = fs::u8path(folderText);
    std::error_code ec;
    if (!fs::is_directory(folder, ec) || ec) {
        std::cout << "That folder does not exist or cannot be accessed.\n";
        return 1;
    }

    std::string reportText;
    std::cout << "HTML report destination (file or folder; Enter for the selected folder): ";
    std::getline(std::cin, reportText);

    fs::path reportPath;
    if (reportText.empty()) {
        reportPath = folder / "filecard-report.html";
    } else {
        reportPath = fs::u8path(reportText);
        // Treat a typed filename or relative path as relative to the selected folder,
        // not Code::Blocks' working directory.
        if (reportPath.is_relative()) {
            reportPath = folder / reportPath;
        }
    }

    // If the user supplies a directory instead of a filename, place the
    // standard report name inside that directory.
    std::error_code destinationTypeError;
    if (fs::is_directory(reportPath, destinationTypeError)) {
        reportPath /= "filecard-report.html";
    }
    if (destinationTypeError) {
        std::cerr << "Could not inspect the report destination: "
                  << destinationTypeError.message() << '\n';
        return 1;
    }

    std::cout << "Report destination: " << reportPath.string() << '\n';

    std::error_code reportCheckError;
    const bool reportAlreadyExists = fs::exists(reportPath, reportCheckError);
    if (reportCheckError) {
        std::cerr << "Could not check the report destination: "
                  << reportCheckError.message() << '\n';
        return 1;
    }

    if (reportAlreadyExists) {
        std::cout << "A file already exists at \"" << reportPath.string()
                  << "\". Overwrite it? (y/N): ";
        std::string answer;
        std::getline(std::cin, answer);
        std::transform(answer.begin(), answer.end(), answer.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });
        if (answer != "y" && answer != "yes") {
            std::cout << "Report not written; the existing file was kept.\n";
            return 0;
        }
    }

    const std::string projectName = folder.filename().empty()
        ? folder.root_path().string()
        : folder.filename().string();

    int fileCount = 0;
    int folderCount = 0;
    int scanErrors = 0;
    std::map<std::string, int> extensionCounts;
    std::set<std::string> projectFiles;
    std::vector<std::pair<std::string, std::string>> licenses;

    // Avoid counting an old report if the chosen output file is inside the scan.
    std::error_code pathError;
    const fs::path absoluteReport = fs::absolute(reportPath, pathError).lexically_normal();
    const bool reportPathResolved = !pathError;

    const std::set<std::string> ignoredDirectories = {
        ".git", ".svn", ".hg", "node_modules", "vendor", "build",
        "dist", "out", "target", "bin", "obj", "debug", "release"
    };

    fs::recursive_directory_iterator it(
        folder, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    if (ec) {
        std::cerr << "Could not start scanning this folder: " << ec.message() << '\n';
        return 1;
    }

    while (it != end) {
        const fs::directory_entry entry = *it;
        const fs::path entryPath = entry.path();
        const bool atProjectRoot = it.depth() == 0;

        std::error_code statusError;
        const fs::file_status status = entry.symlink_status(statusError);
        if (statusError) {
            ++scanErrors;
        } else if (fs::is_directory(status)) {
            if (ignoredDirectories.count(lowercase(entryPath.filename().string())) != 0) {
                it.disable_recursion_pending();
            } else {
                ++folderCount;
            }
        } else if (fs::is_regular_file(status)) {
            bool isReport = false;
            if (reportPathResolved) {
                std::error_code entryPathError;
                const fs::path absoluteEntry = fs::absolute(entryPath, entryPathError).lexically_normal();
                isReport = !entryPathError && absoluteEntry == absoluteReport;
            }

            if (!isReport) {
                ++fileCount;
                const std::string filename = entryPath.filename().string();
                const std::string lowerName = lowercase(filename);
                const std::string extension = lowercase(entryPath.extension().string());

                if (extension == ".cpp" || extension == ".h" ||
                    extension == ".py" || extension == ".js" ||
                    extension == ".ts" || extension == ".go" ||
                    extension == ".rs" || extension == ".java" ||
                    extension == ".html" || extension == ".css" ||
                    extension == ".php") {
                    ++extensionCounts[extension];
                }

                // Project-level markers belong at the selected folder's root.
                if (atProjectRoot) {
                    if (isLicenseFilename(lowerName)) {
                        licenses.push_back(
                            std::make_pair(filename, detectLicenseType(entryPath)));
                    }

                    if (lowerName == "readme" || lowerName.rfind("readme.", 0) == 0 ||
                        lowerName == ".gitignore" || lowerName == "cmakelists.txt" ||
                        lowerName == "makefile" || lowerName == "dockerfile" ||
                        lowerName == "meson.build" || lowerName == "cargo.toml" ||
                        lowerName == "package.json" || lowerName == "go.mod" ||
                        lowerName == "pyproject.toml") {
                        projectFiles.insert(filename);
                    }
                }
            }
        }

        it.increment(ec);
        if (ec) {
            ++scanErrors;
            ec.clear();
        }
    }

    std::cout << "\n========================================\n"
              << "Filecard: " << projectName << "\n"
              << "========================================\n"
              << "Files: " << fileCount << "\n"
              << "Subfolders: " << folderCount << "\n"
              << "File extensions (approximate language indicators):\n";

    if (extensionCounts.empty()) {
        std::cout << "  No recognized extensions\n";
    } else {
        for (const auto& item : extensionCounts) {
            std::cout << "  " << item.first << ": " << item.second << '\n';
        }
    }

    std::cout << "License files:\n";
    if (licenses.empty()) {
        std::cout << "  Not found\n";
    } else {
        for (const auto& license : licenses) {
            std::cout << "  " << license.first << ": " << license.second << '\n';
        }
    }
    std::cout << "Project files:\n";
    if (projectFiles.empty()) {
        std::cout << "  None found\n";
    } else {
        for (const auto& filename : projectFiles) {
            std::cout << "  " << filename << '\n';
        }
    }
    if (scanErrors > 0) {
        std::cout << "Some entries could not be read (" << scanErrors
                  << " scan issue(s)); the report may be incomplete.\n";
    }

    std::ofstream htmlFile(reportPath, std::ios::binary);
    if (!htmlFile) {
        std::cerr << "Could not create the HTML report at: " << reportPath.string() << '\n';
        return 1;
    }

    htmlFile << "<!doctype html>\n<html lang=\"en\">\n<head>\n"
             << "<meta charset=\"utf-8\">\n"
             << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
             << "<title>Filecard - " << htmlEscape(projectName) << "</title>\n"
             << "<style>\n"
             << "*{box-sizing:border-box}body{margin:0;padding:40px 18px;background:#eef2f7;"
                "color:#202938;font:16px/1.5 system-ui,-apple-system,Segoe UI,sans-serif}"
             << "main{max-width:760px;margin:auto;padding:32px;background:#fff;border-radius:18px;"
                "box-shadow:0 12px 36px #18243a18}h1{margin:0;color:#3157c8}h2{margin:8px 0 24px;"
                "overflow-wrap:anywhere}.stats{display:flex;gap:14px;flex-wrap:wrap}.stat{flex:1;"
                "min-width:150px;padding:16px;background:#f2f5ff;border-radius:12px}.number{"
                "display:block;font-size:1.7rem;font-weight:700}section{margin-top:26px}"
                "li{margin:6px 0;overflow-wrap:anywhere}.muted{color:#697386}.warning{"
                "padding:12px;background:#fff4db;border-radius:10px}</style>\n"
             << "</head>\n<body>\n<main>\n"
             << "<h1>Filecard</h1><h2>" << htmlEscape(projectName) << "</h2>\n"
             << "<div class=\"stats\"><div class=\"stat\"><span class=\"number\">"
             << fileCount << "</span>Files</div><div class=\"stat\"><span class=\"number\">"
             << folderCount << "</span>Subfolders</div></div>\n"
             << "<section><h3>File extensions</h3><p class=\"muted\">Approximate language indicators; extension counts do not identify languages with certainty.</p><ul>\n";

    if (extensionCounts.empty()) {
        htmlFile << "<li>No recognized extensions</li>\n";
    } else {
        for (const auto& item : extensionCounts) {
            htmlFile << "<li><code>" << htmlEscape(item.first) << "</code>: "
                     << item.second << "</li>\n";
        }
    }

    htmlFile << "</ul></section>\n<section><h3>License files</h3><ul>\n";

    if (licenses.empty()) {
        htmlFile << "<li>Not found</li>\n";
    } else {
        for (const auto& license : licenses) {
            htmlFile << "<li>" << htmlEscape(license.first) << ": "
                     << htmlEscape(license.second) << "</li>\n";
        }
    }

    htmlFile << "</ul></section>\n<section><h3>Project files</h3><ul>\n";

    if (projectFiles.empty()) {
        htmlFile << "<li>None found</li>\n";
    } else {
        for (const auto& filename : projectFiles) {
            htmlFile << "<li>" << htmlEscape(filename) << "</li>\n";
        }
    }

    htmlFile << "</ul></section>\n";
    if (scanErrors > 0) {
        htmlFile << "<p class=\"warning\">Some entries could not be read ("
                 << scanErrors << " scan issue(s)); this report may be incomplete.</p>\n";
    }
    htmlFile << "</main>\n</body>\n</html>\n";
    htmlFile.close();

    if (!htmlFile) {
        std::cerr << "The report could not be fully written. Check the destination and available disk space.\n";
        return 1;
    }

    std::cout << "HTML report created at: " << reportPath.string() << '\n';
    return 0;
}
