import os
import requests

# Specify the directory to save the downloaded files
download_directory = 'tb'

# Create the directory if it doesn't exist
os.makedirs(download_directory, exist_ok=True)

# Read the tablebases.txt file
with open('tb/tablebases.txt', 'r') as file:
    urls = file.readlines()

# Download each file
for url in urls:
    url = url.strip()  # Remove any leading/trailing whitespace
    if url:  # Check if the URL is not empty
        try:
            # Get the filename from the URL
            filename = os.path.join(download_directory, url.split('/')[-1])

            # Download the file
            response = requests.get(url, allow_redirects=True)

            # Check if the request was successful
            response.raise_for_status()

            # Write the content to a file
            with open(filename, 'wb') as f:
                f.write(response.content)
                
            print(f'Downloaded: {filename}')
        except requests.exceptions.RequestException as e:
            print(f'Failed to download {url}: {e}')