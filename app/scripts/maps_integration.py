import PIL.Image
import time
import pdb
import requests
import matplotlib.pyplot as plt
from matplotlib import animation

# Put the API key in api_key.txt (1 line, no quotes)
with open ("api_key.txt", "r") as myfile:
    api_key=myfile.readlines()[0]

# url variable store url
url = "https://maps.googleapis.com/maps/api/staticmap?"

point_list = []

def fetch_map(latitude,longitude, zoom=10):
    # center defines the center of the map,
    # equidistant from all edges of the map.
    center = "%f,%f" % (latitude, longitude)
    point_list.append(center)

    # zoom defines the zoom
    # level of the map
    # lets make this dynamic?
    # get method of requests module
    # return response object
    r = requests.get(url + "center=" + center + "&markers=color:red|" + center + "&zoom=" + str(zoom) + "&size= 400x400&key=" + api_key + "&sensor=false")
    if (len(r.content) > 1000):
        # wb mode is stand for write binary mode
        f = open('current_map.png', 'wb')


        # r.content gives content,
        # in this case gives image
        f.write(r.content)

        # close mthod of file object
        # save and close the file
        f.close()
        print("Map Updated")
        return True
    else:
        return False


def plot_on_map(latitude_degrees, longitude_degrees):
    print("This function will grab a new map from google maps if needed, and overlay the point on it")
    fetch_map(latitude_degrees, longitude_degrees)
