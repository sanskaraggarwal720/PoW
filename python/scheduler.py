import requests
import schedule
import time

frequency = 1

response = requests.get("http://localhost:8000/start-training")
print(response.text)

def send_api_request():
    url = "http://localhost:8000/predict"
    response = requests.get(url)
    print(response.text)

schedule.every(frequency).minutes.do(send_api_request)

while True:
    schedule.run_pending()
    time.sleep(1)