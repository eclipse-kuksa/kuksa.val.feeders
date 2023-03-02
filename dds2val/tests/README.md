## How to run the tests
1. create virtual python environment (```python3 -m venv testEnv && source testEnv/bin/activate && pip install -r requirements.txt ../src/requirements.txt ../src/requirements-kml.txt```) 
2. terminal 1: ```docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master```
3. terminal 2: ```source testEnv/bin/activate && pytest --html=report.html --self-contained-html --cov=. tests/integration/feeder tests/unit --cov-report html --cov-report xml```