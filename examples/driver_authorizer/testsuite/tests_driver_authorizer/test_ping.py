def test_ping(taxi_driver_authorizer):
    response = taxi_driver_authorizer.get('ping')
    assert response.status_code == 200
    assert response.content == ''
