async def test_boot_logs(
    service_client,
    _service_logfile_path,
):
    with open(_service_logfile_path) as ifile:
        for line in ifile.readlines():
            if 'functional_tests::SlowComponent' not in line:
                continue

            assert 'span=boot/slow-component' in line
            assert 'component_name=slow-component' in line
            return
    assert False
