import logging

LOG_FORMAT = "%(levelname)s: %(message)s"
LOG_LEVEL = logging.INFO


def init_logging() -> None:
    """Initialize the logging module. Handlers and Levels are set on module-base logger."""
    logger = logging.getLogger(__name__)
    logger.setLevel(LOG_LEVEL)
    handler = logging.StreamHandler()
    formatter = logging.Formatter(LOG_FORMAT)
    handler.setFormatter(formatter)
    logger.addHandler(handler)


init_logging()
