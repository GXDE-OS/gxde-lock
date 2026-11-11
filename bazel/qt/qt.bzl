"""Minimal Qt6 rule set

Provides:
  lock_configure_file - CMake-style @VAR@ / ${VAR} template substitution.
  qt_moc             - run moc over headers -> moc_*.cpp (added to a target's srcs).
  qt_rcc             - compile a .qrc -> qrc_*.cpp.
  qt_qml_module      - compile QML files into a qrc_*.cpp QML module.
  qt_dbus_adaptor    - qdbusxml2cpp adaptor (.h/.cpp) from a D-Bus XML.
  qt_dbus_interface  - qdbusxml2cpp interface/proxy (.h/.cpp) from a D-Bus XML.
  qt_lrelease        - compile a .ts -> .qm.
  qt_cc_binary       - cc_binary that auto-mocs `moc_hdrs` and links //bazel/qt:qt6.
  qt_cc_library      - cc_library that auto-mocs `moc_hdrs` and deps //bazel/qt:qt6.
"""

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//bazel/qt:constants.bzl", "CONSTANTS")

# Host Qt6 tool locations (Debian trixie layout).
MOC = "/usr/lib/qt6/libexec/moc"
RCC = "/usr/lib/qt6/libexec/rcc"
QDBUSXML2CPP = "/usr/lib/qt6/bin/qdbusxml2cpp"
LRELEASE = "/usr/lib/qt6/bin/lrelease"

QT6 = "//bazel/qt:qt6"

def _basename(p):
    return p.split("/")[-1]

def _stem(p):
    b = _basename(p)
    return b[:b.rindex(".")] if "." in b else b

def lock_configure_file(name, src, out, extra = {}):
    """Substitute @VAR@ and ${VAR} from CONSTANTS (+ extra) like CMake configure_file."""
    subs = dict(CONSTANTS)
    subs.update(extra)
    seds = []
    for k, v in subs.items():
        # Values are plain paths/ints with no '|', safe as sed delimiter.
        seds.append("-e 's|@%s@|%s|g'" % (k, v))
        seds.append("-e 's|$${%s}|%s|g'" % (k, v))
    native.genrule(
        name = name,
        srcs = [src],
        outs = [out],
        cmd = "sed %s $< > $@" % " ".join(seds),
    )

def qt_moc(name, hdrs, moc_opts = []):
    """Generate moc_<stem>.cpp for each header. Returns the list of generated srcs."""
    outs = []
    for h in hdrs:
        stem = _stem(h)
        rule = "%s_%s" % (name, stem)
        out = "moc_%s.cpp" % stem
        native.genrule(
            name = rule,
            srcs = [h],
            outs = [out],
            cmd = "%s %s $(location %s) -o $@" % (MOC, " ".join(moc_opts), h),
        )
        outs.append(out)
    return outs

def qt_moc_cpp(name, srcs, moc_opts = []):
    """For .cpp files that define Q_OBJECT classes and `#include "X.moc"`, run moc
    over the .cpp to produce X.moc, and wrap the .moc files in a cc_library
    (textual_hdrs) on the package include path. Returns the library label to add
    to a target's `deps`."""
    outs = []
    for s in srcs:
        stem = _stem(s)
        rule = "%s_%s" % (name, stem)
        out = "%s.moc" % stem
        native.genrule(
            name = rule,
            srcs = [s],
            outs = [out],
            cmd = "%s %s $(location %s) -o $@" % (MOC, " ".join(moc_opts), s),
        )
        outs.append(out)
    cc_library(
        name = name,
        textual_hdrs = outs,
        includes = ["."],
    )
    return ":" + name

def qt_moc_textual(name, hdrs, moc_opts = []):
    """Like qt_moc, but the generated moc_<stem>.cpp is exposed as a textual header
    (for the rare .cpp that does `#include "moc_<stem>.cpp"` itself). Returns a
    cc_library label to add to `deps`. These must NOT also appear in `moc_hdrs`."""
    outs = []
    for h in hdrs:
        stem = _stem(h)
        rule = "%s_%s" % (name, stem)
        out = "moc_%s.cpp" % stem
        native.genrule(
            name = rule,
            srcs = [h],
            outs = [out],
            cmd = "%s %s $(location %s) -o $@" % (MOC, " ".join(moc_opts), h),
        )
        outs.append(out)
    cc_library(
        name = name,
        textual_hdrs = outs,
        includes = ["."],
    )
    return ":" + name

def qt_rcc(name, qrc, data, init_name):
    """Compile a .qrc into qrc_<init_name>.cpp. `data` are the referenced files."""
    out = "qrc_%s.cpp" % init_name
    native.genrule(
        name = name,
        srcs = [qrc] + data,
        outs = [out],
        cmd = "%s --name %s $(location %s) -o $@" % (RCC, init_name, qrc),
    )
    return out

def qt_qml_module(name, uri, version, qml_files, init_name):
    """Compile QML files into qrc_<init_name>.cpp as module `uri`, laid out like
    qt_add_qml_module (qrc:/qt/qml/<uri>/ with a qmldir) so that
    QQmlApplicationEngine::loadFromModule finds them."""
    prefix = "/qt/qml/" + uri.replace(".", "/")
    bases = [_basename(f) for f in qml_files]
    qmldir = ["module " + uri, "prefer :%s/" % prefix] + [
        "%s %s %s" % (_stem(b), version, b)
        for b in bases
    ]
    qrc = ['<RCC><qresource prefix="%s">' % prefix, "<file>qmldir</file>"] + [
        "<file>%s</file>" % b
        for b in bases
    ] + ["</qresource></RCC>"]
    out = "qrc_%s.cpp" % init_name
    native.genrule(
        name = name,
        srcs = qml_files,
        outs = [out],
        cmd = " && ".join([
            "d=$$(mktemp -d)",
            "cp $(SRCS) $$d/",
            "printf '%%s\\n' %s > $$d/qmldir" % " ".join(["'%s'" % l for l in qmldir]),
            "printf '%%s\\n' %s > $$d/module.qrc" % " ".join(["'%s'" % l for l in qrc]),
            "%s --name %s $$d/module.qrc -o $@" % (RCC, init_name),
            "rm -rf $$d",
        ]),
    )
    return out

def qt_dbus_adaptor(name, xml, parent_header, parent_class, basename):
    """Generate a D-Bus adaptor (<basename>.h/.cpp) that delegates to parent_class."""
    native.genrule(
        name = name,
        srcs = [xml],
        outs = [basename + ".h", basename + ".cpp"],
        cmd = "%s -a $(RULEDIR)/%s -i %s -l %s $(location %s)" % (
            QDBUSXML2CPP,
            basename,
            parent_header,
            parent_class,
            xml,
        ),
    )
    return basename + ".cpp"

def qt_dbus_interface(name, xml, basename, includes = []):
    """Generate a D-Bus interface/proxy (<basename>.h/.cpp)."""
    inc = " ".join(["-i %s" % i for i in includes])
    native.genrule(
        name = name,
        srcs = [xml],
        outs = [basename + ".h", basename + ".cpp"],
        cmd = "%s -p $(RULEDIR)/%s %s $(location %s)" % (
            QDBUSXML2CPP,
            basename,
            inc,
            xml,
        ),
    )
    return basename + ".cpp"

def qt_lrelease(name, ts, out = None):
    """Compile a .ts translation into a .qm."""
    out = out or (_stem(ts) + ".qm")
    native.genrule(
        name = name,
        srcs = [ts],
        outs = [out],
        cmd = "%s $(location %s) -qm $@" % (LRELEASE, ts),
    )
    return out

def qt_lrelease_all(name, ts_files):
    """Compile a list of .ts files to .qm and gather them in a filegroup `name`."""
    qms = []
    for ts in ts_files:
        stem = _stem(ts)
        rule = "%s_%s" % (name, stem.replace("@", "_at_"))
        qms.append(qt_lrelease(name = rule, ts = ts))
    native.filegroup(name = name, srcs = qms)

def qt_cc_library(name, srcs = [], hdrs = [], moc_hdrs = [], moc_opts = [], deps = [], **kwargs):
    moc_srcs = qt_moc(name + "_moc", moc_hdrs, moc_opts) if moc_hdrs else []
    cc_library(
        name = name,
        srcs = srcs + moc_srcs,
        hdrs = hdrs,
        deps = deps + [QT6],
        includes = kwargs.pop("includes", ["."]),
        **kwargs
    )

def qt_cc_binary(name, srcs = [], moc_hdrs = [], moc_opts = [], deps = [], **kwargs):
    moc_srcs = qt_moc(name + "_moc", moc_hdrs, moc_opts) if moc_hdrs else []
    cc_binary(
        name = name,
        srcs = srcs + moc_srcs,
        deps = deps + [QT6],
        includes = kwargs.pop("includes", ["."]),
        **kwargs
    )
