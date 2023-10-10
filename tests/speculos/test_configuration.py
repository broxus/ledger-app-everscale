from time import sleep

def test_configuration(cmd):
    sleep(0.5)
    if cmd.model == "nanos":
        assert cmd.get_configuration() == (1, 0, 9) #major, minor, patch
    if cmd.model == "nanox":
        assert cmd.get_configuration() == (1, 0, 9)

