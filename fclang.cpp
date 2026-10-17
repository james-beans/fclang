/*
 * Copyright (c) 2026- James-beans
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL version 2 was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/ .
 */

#if !((defined(_MSVC_LANG) ? _MSVC_LANG : __cplusplus) >= 202302L)
  #error "fclang / fclang++ requires at least C++23 to build."
#endif

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#if defined(__APPLE__)
  #include <mach-o/dyld.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
#elif defined(__linux__)
  #include <unistd.h>
#else
  #error "fclang / fclang++ requires MacOS, FreeBSD / OpenBSD / NetBSD or Linux (any) to build."
#endif

#include <sys/wait.h>

using std::string;
using std::string_view;
using std::stringstream;
using std::array;
using std::unique_ptr;

using std::cout;
using std::cerr;

namespace fs = std::filesystem;

namespace fclang {

  bool g_IsCpp = false;
  bool g_IsColorSupported = true;

  void detect_color_mode() {
    g_IsColorSupported = true;
    const char* no_color = getenv("NO_COLOR");
    if (no_color != nullptr && no_color[0] != '\0') {
      g_IsColorSupported = false;
    }
  }

  void detect_compiler_mode(string_view program_name) {
    // If compiled explicitly with a C++ flag, force it true
    // Otherwise, inspect argv[0] dynamically at runtime
    #ifdef FCLANG_FORCE_CPP
      (void)program_name;
      g_IsCpp = true;
    #else
      g_IsCpp = (program_name.find("++") != string_view::npos);
    #endif
  }

  string get_color_flag() {
    return g_IsColorSupported ? " -fcolor-diagnostics" : " -fno-color-diagnostics";
  }

  template <typename... Args>
  void error(Args&&... args) {
    // \033[0mclang: \033[0;1;31merror: \033[0m\033[1mno input files\033[0m

    string_view prog = g_IsCpp ? "fclang++" : "fclang";

    if (g_IsColorSupported) {
      cerr << "\033[0m" << prog << ": \033[0;1;31merror: \033[0m\033[1m";
      (cerr << ... << args);
      cerr << "\033[0m\n";
    } else {
      cerr << prog << ": error: ";
      (cerr << ... << args);
      cerr << "\n";
    }
  }

  string find_in_path(string_view filename) {
    const char* path_env = getenv("PATH");
    if (!path_env) { return ""; } // Placeholder

    char path_sep = ':';

    stringstream ss(path_env);
    string item;
    while (getline(ss, item, path_sep)) {
      fs::path full_path = fs::path(item) / filename;
      if (fs::exists(full_path)) {
        return full_path.parent_path().string();
      }
    }

    return ""; // Placeholder
  }

  string get_dir(const char* argv) {
    string path_str = "";

    // If argv[0] has a slash, resolve it immediately (fastest path-based lookup)
    if (argv != nullptr && argv[0] != '\0') {
      fs::path p(argv);
      // Wtf? Why so long? -_-
      if (p.has_relative_path() && (string(argv).find('/') != string::npos || string(argv).find('\\') != string::npos)) {
        // Use lexically_normal() to clean up any ./ components taking parent_path
        return fs::absolute(p).lexically_normal().parent_path().string();
      }
    }

    // Use OS-specific kernel calls if argv[0] was just a bare command name
#if defined(__APPLE__)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
      path_str = buffer;
    }
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 }; // Men In Black 4?
    char buffer[1024];
    size_t size = sizeof(buffer);
    if (sysctl(mib, 4, buffer, &size, NULL, 0) == 0) {
      path_str = buffer;
    }
#elif defined(__OpenBSD__)
    char buffer[1024];
    if (getexecpath(buffer, sizeof(buffer)) == 0) {
      path_str = buffer;
    }
#elif defined(__linux__)
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
      buffer[len] = '\0';
      path_str = buffer;
    }
#endif

    if (!path_str.empty()) {
      return fs::path(path_str).lexically_normal().parent_path().string();
    }

    // Last resort fallback to environment PATH scan
    if (argv != nullptr && argv[0] != '\0') {
      return find_in_path(argv);
    }

    return ""; // Fallback
  }

  bool replace_all(string& str, string_view from, string_view to) {
    if (from.empty()) { return false; }
    size_t start_pos = 0;
    bool modified = false;
    while ((start_pos = str.find(from, start_pos)) != string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
      modified = true;
    }
    return modified;
  }

  string exec_command(const string& cmd, int& exit_code_out) {
    string result = "";

    array<char, 128> buffer;
    string full_cmd = cmd + " 2>&1";

    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) {
      exit_code_out = 127;
      return ""; // Placeholder
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      result += buffer.data();
    }

    int status = pclose(pipe);
    if (WIFEXITED(status)) { // How the hell did you find a wife in the first place?
      exit_code_out = WEXITSTATUS(status);
    } else {
      exit_code_out = 1;
    }

    return result;
  }

  namespace handle {

    void help(const string& real_compiler) {
      int ec = 0;
      string output = exec_command(real_compiler + get_color_flag() + " --help", ec);

      // Apply branding replacements across all lines
      replace_all(output, "clang++", "fclang++");
      replace_all(output, "clang", "fclang");
      replace_all(output, "CLANG", "FCLANG");

      cout << output;
    }

    void version(const string& real_compiler, const char* argv, const string& default_target) {
      int ec = 0;
      string output = exec_command(real_compiler + get_color_flag() + " --version", ec);

      string target_name = g_IsCpp ? "fclang++" : "fclang";
      replace_all(output, "clang version", target_name + " version");

      stringstream ss(output);
      string line;
      string new_output = "";

      string resolved_dir = get_dir(argv);

      while (getline(ss, line)) {
        if (line.rfind("Target:", 0) == 0) {
          new_output += "Target: " + default_target + "\n";
        } else if (line.rfind("InstalledDir:", 0) == 0) {
          new_output += "InstalledDir: " + resolved_dir + "\n";
        } else {
          new_output += line + "\n";
        }
      }

      cout << new_output;
    }

  } // namespace handle
} // namespace fclang

int main(int argc, char* argv[]) {
  fclang::detect_compiler_mode(argv[0]);
  fclang::detect_color_mode();

  string real_compiler = fclang::g_IsCpp ? "clang++" : "clang";
  string default_target = "armv7-unknown-linux-gnueabi";
  string user_args = "";

  for (int i = 1; i < argc; ++i) {
    string_view arg = argv[i];

    if (arg == "--help") {
      fclang::handle::help(real_compiler);
      return 0;
    } else if (arg == "--version") {
      fclang::handle::version(real_compiler, argv[0], default_target);
      return 0;
    } else if (arg == "-static") {
      fclang::error("static linking is forced automatically");
      return 1;
    } else if (arg == "--target") {
      if (i + 1 >= argc) {
        fclang::error("no target specified for '--target'");
        return 1;
      }
      string target_val = argv[++i];
      if (target_val.empty()) {
        fclang::error("no target specified for '--target'");
        return 1;
      }

      // Kindle Paperwhite 1-2, Voyage, Oasis 1
      if (target_val == "legacy" || target_val == "kindle-legacy") {
        default_target = "armv7-unknown-linux-gnueabi";

      // Kindle Paperwhite 3, Oasis 1 (i.MX6)
      } else if (target_val == "pw3" || target_val == "kindle-pw3") {
        default_target = "armv7-unknown-linux-gnueabihf";

      // Kindle Paperwhite 4-5, 10-11
      } else if (target_val == "modern" || target_val == "kindle-modern") {
        default_target = "armv7-unknown-linux-gnueabi";

      // Kindle Scribe (2022/2024), Colorsoft
      } else if (target_val == "scribe" || target_val == "kindle-scribe") {
        default_target = "aarch64-unknown-linux-gnu";

      // Kindle Keyboard, Touch, 4-5
      } else if (target_val == "old" || target_val == "kindle-old") {
        default_target = "arm-unknown-linux-gnueabi";

      } else {
        fclang::error("invalid target '", target_val, "'");
        return 1;
      }
      continue;
    } else if (arg.rfind("--target=", 0) == 0) {
      string_view target_val = arg.substr(9);
      if (target_val.empty()) {
        fclang::error("no target specified for '--target'");
        return 1;
      }

      // Kindle Paperwhite 1-2, Voyage, Oasis 1
      if (target_val == "legacy" || target_val == "kindle-legacy") {
        default_target = "armv7-unknown-linux-gnueabi";

      // Kindle Paperwhite 3, Oasis 1 (i.MX6)
      } else if (target_val == "pw3" || target_val == "kindle-pw3") {
        default_target = "armv7-unknown-linux-gnueabihf";

      // Kindle Paperwhite 4-5, 10-11
      } else if (target_val == "modern" || target_val == "kindle-modern") {
        default_target = "armv7-unknown-linux-gnueabi";

      // Kindle Scribe (2022/2024), Colorsoft
      } else if (target_val == "scribe" || target_val == "kindle-scribe") {
        default_target = "aarch64-unknown-linux-gnu";

      // Kindle Keyboard, Touch, 4-5
      } else if (target_val == "old" || target_val == "kindle-old") {
        default_target = "arm-unknown-linux-gnueabi";

      } else {
        fclang::error("invalid target '", target_val, "'");
        return 1;
      }
      continue;
    }

    user_args += " ";
    user_args += argv[i];
  }

  // Build the compilation command, checking color diagnostics so piped output stays colorful or not
  string final_cmd = real_compiler + fclang::get_color_flag();

  // Append user arguments, followed by our automatic Kindle target, and static flags at the end
  final_cmd += user_args;
  final_cmd += " --target=";
  final_cmd += default_target;
  final_cmd += " -static";
  final_cmd += " -fuse-ld=lld";

  int exit_code = 0;
  string output = fclang::exec_command(final_cmd, exit_code);

  // Scrub any lingering "clang" references across all compiler diagnostics
  if (!fclang::replace_all(output, "clang++", "fclang++")) {
    fclang::replace_all(output, "clang", "fclang");
  }
  fclang::replace_all(output, "CLANG", "FCLANG");

  if (!output.empty()) {
    cerr << output;
  }

  return exit_code;
}