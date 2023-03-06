import pytest


async def test_unknown_tasks(service_client):
    with pytest.raises(service_client.TestsuiteTaskNotFound):
        await service_client.run_task('unknown-task')


async def test_task_run(service_client, testpoint):
    @testpoint('sample-task/action')
    def task_action(data):
        pass

    # /// [run_task]
    await service_client.run_task('sample-task')
    assert task_action.times_called == 1
    # /// [run_task]


async def test_task_spawn(service_client, testpoint):
    @testpoint('sample-task/action')
    def task_action(data):
        pass

    # /// [spawn_task]
    async with service_client.spawn_task('sample-task'):
        await task_action.wait_call()
    # /// [spawn_task]


async def test_list_tasks(service_client):
    tasks = await service_client.list_tasks()
    assert 'sample-task' in tasks
