import requests
from concurrent.futures import ThreadPoolExecutor

def make_request(url):
  response = requests.get(url)
  print(f"Response from {url}: {response.status_code}")

if __name__ == "__main__":
  urls = ["http://localhost:2020/404.html"] * 1000  # Change the number to adjust the number of concurrent requests

  with ThreadPoolExecutor() as executor:
    executor.map(make_request, urls)