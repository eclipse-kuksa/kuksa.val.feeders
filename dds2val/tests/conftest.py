import pytest
from py.xml import html


def pytest_html_report_title(report):
    """modifying the title  of html report"""
    report.title = "Integration Test Report"


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    """data from the output of pytest gets processed here
    and are passed to pytest_html_results_table_row"""
    pytest_html = item.config.pluginmanager.getplugin("html")
    outcome = yield
    report = outcome.get_result()
    extra = getattr(report, "extra", [])
    # this is the output that is seen end of test case
    report = outcome.get_result()

    if report.when == "call":
        # always add url/text to report
        extra.append(pytest_html.extras.text(""))
