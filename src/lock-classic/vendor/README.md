# Vendored backend
GXDE-Display-Manager致力于成为一个开箱即用的登录管理器，所以我们集成了包括但不限于SDDM和以前GXDE用的旧版DDE LightDM Greeter的移植。

GXDE-Display-Manager aims to provide an out-of-box experience that everything's done after installing the login manager itself. Hence, we integrated things including SDDM, the port of legacy Deepin LightDM greeter, etc.

然而，我们发现旧版DDE LightDM Greeter依赖于`dde-daemon`，考虑到我们正在废弃旧的`dde-daemon`而新的`gxde-daemon`尚未准备好（同时我们希望`gxdm`支援更多系统），我们决定去除Greetr对`dde-daemon`的依赖。

Sadly, we found out that the legacy DDE LightDM Greeter highly relys on the `dde-daemon`. Considering that we're currently rewriting old `dde-daemon` and the new package `gxde-daemon` is NOT ready yet (btw, we hope that `gxdm` supports more distros beyond GXDE OS), we're removing greetr's dependency on `dde-daemon`.

于是，我们引入了这个库，这是[CharOfString](https://gitee.com/MarcusP)编写的[Samoyed Greeter](https://gitee.com/MarcusP/samoyed-greeter)的部分源码，我们将使用其`backend`提供的用户解析来代替当前greeter对`dde-daemon`的依赖。

... that's the reason why we introduced this library. This is partial source code of [CharOfString](https://gitee.com/MarcusP)'s [Samoyed Greeter](https://gitee.com/MarcusP/samoyed-greeter). We used its `backend` to replace those D-Bus interfaes related to the legacy `dde-daemon`.

---

原项目使用Google-Styled C++，我们没有修改其格式。

The original project is using Google-Styled C++, we kept its formatting as-is here.