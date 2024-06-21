from datetime import datetime
from ricxappframe.logger.mdclogger import MDCLogger, Level


def get_logger() -> MDCLogger:
    # log configs
    logger_configs = {
        "name": "iqos-xapp",
        "xapp_version": "0.0.1",
        "time": datetime.now().strftime("%Y-%m-%dT%H:%M:%S"),
        "level": Level.DEBUG
    }

    # set configs
    logger = MDCLogger(
        name=logger_configs.get("name"),
        level=logger_configs.get("level")
    )

    logger.mdclog_format_init(configmap_monitor=True)
    logger.add_mdc("VERSION", logger_configs.get("xapp_version"))
    logger.add_mdc("TIME", logger_configs.get("time"))

    return logger
