print('importing libraries...')
from flask import Flask, request, jsonify
import logging
import random
import time
import torch
import colorize
import utils
import base64

from PIL import Image
import requests, os
from io import BytesIO

from resnet import resnet18
import torch.nn as nn
from transformer_net import TransformerNet

from settings import *

print('done!\nlaunching the server...')

# load the models

#HasWhiteBalanceModel = torch.jit.load('model/jit_color.pt')
HasWhiteBalanceModel = torch.load('model/color.mdl', map_location=torch.device('cpu'))

net = resnet18(pretrained=True)
net.fc = nn.Sequential(nn.Linear(512, 1000), nn.ReLU(), nn.Linear(1000, 1000), nn.ReLU(), nn.Linear(1000, 100),
                       nn.ReLU(), nn.Linear(100, 1), nn.Sigmoid())

net.load_state_dict(HasWhiteBalanceModel["state_dict"])
net.eval()
net.to('cpu')

# load model to correct white balance
WhiteBalanceModel = TransformerNet()
#state_dict = torch.jit.load('model/jit_matsumoto.pt')
state_dict = torch.load('model/matsumoto.mdl')

WhiteBalanceModel = nn.DataParallel(WhiteBalanceModel)
WhiteBalanceModel.load_state_dict(state_dict)
WhiteBalanceModel.cpu()


app = Flask(__name__)
@app.route("/")
def hello():
    return "Image classification example\n"


@app.route('/processImage', methods=['POST'])
def predict():
    try:
        json = request.get_json()
    except Exception as err:
        print("err")
        print(err)

    t = time.time()
    dt = time.time() - t
    
    # print(json['image'])

    try:
        img = utils.load_image(BytesIO(base64.b64decode(json['image'])))
    except Exception as err:
        print("err")
        print(err)

    b64image = base64.b64encode(colorize.eval_service(img, net, WhiteBalanceModel))

    response = {"image":b64image}
    print(response)

    print('done')
    app.logger.info("Execution time: %0.02f seconds" % (dt))
    return jsonify(response)

if __name__ == '__main__':
    app.run(host="0.0.0.0", debug=True, port=PORT)

