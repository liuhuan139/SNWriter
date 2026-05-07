#include "BinaryPatcher.hpp"
#include "MainWindow.hpp"

#include <gtkmm/application.h>

#include <cstdlib>
#include <sys/wait.h>

namespace {
std::string shell_quote_single(const std::string& s) {
    std::string out = "'";
    out.reserve(s.size() + 2);
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

int worker_write_device_main(int argc, char** argv) {
    // argv: [0]=exe [1]=--worker [2]=write_device [3]=local_proinfo
    if (argc != 4) {
        return 2;
    }
    const std::string local = argv[3];

    // 1) push 到 /sdcard/proinfo
    // 2) dd 写入 /dev/block/by-name/proinfo
    // 3) 删除 /sdcard/proinfo 并 reboot 使修改生效
    const std::string cmd =
        "adb push " + shell_quote_single(local) + " /sdcard/proinfo && "
        "adb shell \"dd if=/sdcard/proinfo of=/dev/block/by-name/proinfo\" && "
        "adb shell \"rm -f /sdcard/proinfo\" && "
        "adb reboot";

    const int rc = std::system(cmd.c_str());
    if (rc == -1) {
        return 1;
    }
    if (WIFEXITED(rc)) {
        return WEXITSTATUS(rc) == 0 ? 0 : 1;
    }
    return 1;
}

int worker_export_android_main(int argc, char** argv) {
    // argv: [0]=exe [1]=--worker [2]=export_android [3]=output_path
    if (argc != 4) {
        return 2;
    }
    const std::string out_path = argv[3];

    auto try_export = [&out_path]() -> bool {
        const std::string cmd =
            "mkdir -p /tmp/snedit && "
            "adb pull /dev/block/by-name/proinfo " + shell_quote_single(out_path);
        const int rc = std::system(cmd.c_str());
        if (rc == -1) {
            return false;
        }
        if (!WIFEXITED(rc) || WEXITSTATUS(rc) != 0) {
            return false;
        }
        // 检查文件是否存在且非空
        const std::string check_cmd = "test -s " + shell_quote_single(out_path);
        const int check_rc = std::system(check_cmd.c_str());
        return WIFEXITED(check_rc) && WEXITSTATUS(check_rc) == 0;
    };

    if (try_export()) {
        return 0;
    }

    std::system("adb root");
    std::system("sleep 1.5");

    if (try_export()) {
        return 0;
    }

    return 1;
}

int worker_main(int argc, char** argv) {
    // argv: [0]=exe [1]=--worker [2]=mode [...]
    if (argc < 3) {
        return 2;
    }

    const std::string mode = argv[2];
    if (mode == "patch") {
        // argv: [0]=exe [1]=--worker [2]=patch [3]=source [4]=dest [5]=sn
        if (argc != 6) {
            return 2;
        }
        const std::string source = argv[3];
        const std::string dest = argv[4];
        const std::string sn = argv[5];
        const PatchResult result = patch_binary_with_sn(source, dest, sn);
        return result.error == PatchError::Ok ? 0 : 1;
    }
    if (mode == "write_device") {
        return worker_write_device_main(argc, argv);
    }
    if (mode == "export_android") {
        return worker_export_android_main(argc, argv);
    }
    return 2;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--worker") {
        return worker_main(argc, argv);
    }

    auto app = Gtk::Application::create(argc, argv, "com.hexedit.snpatcher");
    MainWindow window(argv[0]);
    return app->run(window);
}
