from pathlib import Path
import pytest

from speculos.client import SpeculosClient

from ledger_client.ledger_cmd import LedgerCommand


SCRIPT_DIR = Path(__file__).absolute().parent
API_URL = "http://127.0.0.1:5000"
#http://10.66.66.8:5000


def pytest_addoption(parser):
    # nanos, nanox, nanosp
    parser.addoption("--model", action="store", default="nanos")
    # qt: default, requires a X server
    # headless: nothing is displayed
    parser.addoption("--display", action="store", default="qt")

    path: str = SCRIPT_DIR.parent.parent / "bin" / "app.elf"
    parser.addoption("--path", action="store", default=path)

@pytest.fixture()
def client(pytestconfig):
    file_path = pytestconfig.getoption("path")
    model = pytestconfig.getoption("model")

    args = ['--log-level', 'speculos:DEBUG','--model', model, '--display', pytestconfig.getoption("display")]
    with SpeculosClient(app=str(file_path), args=args) as client:
        yield client


@pytest.fixture()
def cmd(client, pytestconfig):
    yield LedgerCommand(
        client=client,
        debug=True,
        model=pytestconfig.getoption("model"),
    )