import os


def iter_dir(walkdir):
    for dirpath, dirnames, filenames in os.walk(walkdir):
        for f in dirnames + filenames:
            fullpath = os.path.join(dirpath, f)
            if os.path.isfile(fullpath):
                yield fullpath


def rm_win32_code(path):
    tmp_path = "%s.tmp" % path
    with open(tmp_path, "w") as wf:
        win32_skip = False
        with open(path) as rf:
            for line in rf:
                if line.startswith("#ifdef _WIN32"):
                    win32_skip = True
                    wf.write(line.rstrip())  # rm dos "\r\n"
                    wf.write('\n')  # unix "\r\n"
                    continue
                if line.startswith("#else") or line.startswith("#elif") or line.startswith("#endif"):
                    win32_skip = False
                if win32_skip:
                    continue
                wf.write(line.rstrip())  # rm dos "\r\n"
                wf.write('\n')  # unix "\r\n"
    os.rename(tmp_path, path)


if __name__ == '__main__':
    for path in iter_dir("plog"):
        rm_win32_code(path)
