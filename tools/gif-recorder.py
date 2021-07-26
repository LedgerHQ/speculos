#!/usr/bin/env python3

"""
Record an emulated device screen and save the animation as a gif.
"""

import argparse
import io
import logging
import requests
from PIL import Image


def take_screenshot(session, api_url):
    response = session.get(f"{api_url}/screenshot")
    assert response.status_code == 200
    return response.content


def record_images(api_url):
    images = []

    logging.info("press CTRL-C to stop recording")

    try:
        with requests.Session() as session:
            with session.get(f"{api_url}/events?stream=true", stream=True) as stream:
                while True:
                    image = take_screenshot(session, api_url)
                    if len(images) == 0 or image != images[-1]:
                        images.append(image)
                    line = stream.raw.readline()
                    logging.debug(f"got event: {line}")
                    if not line:
                        break
    except KeyboardInterrupt:
        pass

    return images


def save_gif(outfile, images, duration=500):
    logging.info(f"saving images to {outfile}")
    images = [Image.open(io.BytesIO(image)) for image in images]
    images[0].save(outfile, save_all=True, append_images=images[1:], duration=duration, loop=0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Record a screen as a GIF.")
    parser.add_argument("--api-url", default="http://127.0.0.1:5000", help="REST API URL")
    parser.add_argument("--outfile", default="/tmp/speculos.gif", help="Output file")
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    images = record_images(args.api_url)
    save_gif(args.outfile, images)
