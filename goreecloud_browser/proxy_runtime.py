"""Qt proxy adapter. Must run before constructing any QWebEngineProfile."""
from PySide6.QtNetwork import QNetworkProxy, QNetworkProxyFactory
from goreecloud_browser.proxy import ProxyConfig


def apply_global_proxy(config: ProxyConfig) -> None:
    if config.mode == "system":
        QNetworkProxyFactory.setUseSystemConfiguration(True)
    elif config.mode == "direct":
        QNetworkProxy.setApplicationProxy(QNetworkProxy(QNetworkProxy.ProxyType.NoProxy))
    elif config.mode == "http":
        QNetworkProxy.setApplicationProxy(
            QNetworkProxy(QNetworkProxy.ProxyType.HttpProxy, config.host, config.port)
        )
    elif config.mode == "socks5":
        QNetworkProxy.setApplicationProxy(
            QNetworkProxy(QNetworkProxy.ProxyType.Socks5Proxy, config.host, config.port)
        )
    else:
        raise ValueError("Unsupported proxy mode")
