#
#  &copy; Cormac Guerin , compdeep G.K
#  Copyright 2021 All Rights Reserved.
#
#  All information contained herein is, and remains
#  the property of Cormac Guerin & compdeep G.K
#
#  The intellectual and technical concepts within remain
#  the sole property of the aforementioned owners.
#
#  Reproduction and / or distriptions of this software is 
#  strictly prohibited.

import argparse
import torch
import torchvision
import torch.nn as nn
import torchvision.transforms as transforms
import math
import utils
import numpy as np
import os
import os.path as osp
import sys
import time
import cv2
import colorsys

from torch.utils.data import Dataset
from torchvision import transforms
from skimage import io, color, filters
from PIL import Image, ImageEnhance, ImageStat
from torch.utils.data import DataLoader, dataset
from transformer_net import TransformerNet
from datetime import datetime
from resnet import resnet18


def analyse(img):

    # Initialize to check if HSV min/max value changes
    img = np.array(img)
    img = img[:, :, ::-1].copy() 

    # skin mask  //  TODO
    skin_lower = np.array([0,20,150])
    skin_upper = np.array([23,111,255])

    # the brightest colors
    white_lower = np.array([0,0,230])
    white_upper = np.array([0,255,255])

    # 18# grey #777777
    grey_lower = np.array([0,0,109])
    grey_upper = np.array([0,255,133])

    black = np.array([0,0,0])
    white = np.array([255,255,255])
    midgrey = np.array([119,119,119])

    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV_FULL)
    hue, sat, val = cv2.split(hsv)
    # brightness
    brightness = np.mean(val)
    # saturation
    saturation = np.mean(sat)

    # masks
    # skin_mask = cv2.inRange(hsv, skin_lower, skin_upper)
    # white_mask = cv2.inRange(hsv, white_lower, white_upper)
    # grey_mask = cv2.inRange(hsv, grey_lower, grey_upper)
    # black_mask = cv2.inRange(mask, black, black)

    # white mask covers the whitest areas of the image.
    # we use this to approximate the average color of the whites.
    # this allows use to roughly estimate the white balance.
    # maskW = cv2.bitwise_and(hsv, hsv, mask= white_mask) 
    # maskW = cv2.cvtColor(maskW, cv2.COLOR_HSV2BGR)
    # meanW = cv2.mean(maskW, white_mask)

    # maskG = cv2.bitwise_and(hsv, hsv, mask= grey_mask) 
    # maskG = cv2.cvtColor(maskG, cv2.COLOR_HSV2BGR)
    # meanG = cv2.mean(maskG, grey_mask)

    # brightness
    # brightness = hsv[...,2].mean()

    # saturation
    # saturation = hsv[...,1].mean()


    # print("meanW")
    # print(meanW)
    # print("meanG")
    # print(meanG)
    # white_mean = np.array([meanW[0],meanW[1],meanW[2]])
    # grey_mean = np.array([meanG[0],meanG[1],meanG[2]])

    # Difficult to find a method that can reliably determine the white balance. Needs work
    #
    # The idea here is that if there is a similar color tone deviation in both the whites
    # and the greys then there is more likely to be a color balance issue.
    # In that case the deviation would be proportional to the scale of the white/grey balance 
    # shift.
    # If there is a wide deviation then it's likely unrelated to color balance.
    # note: np.std seems to flatten the array which gives incorrect results.
    # a = (white - white_mean) / (midgrey - grey_mean)
    # white_balance = np.sqrt(np.sum( [ a[0]**2, a[1]**2, a[2]**2 ] ) / 2)
    # white_balance = np.sum((white - white_mean) + (midgrey - grey_mean)) / white_balance

    # white_balance = white / np.array([meanW])

    white_balance = grey_edge(img)

    stat = {'brightness':brightness,'saturation':saturation,'white_balance':white_balance}

    print(stat)

    # DEBUG COLORS
    #
    # meanW = Image.new('RGB', (300, 200), (int(meanW[2]),int(meanW[1]),int(meanW[0])))
    # meanW.show()
    # meanG = Image.new('RGB', (300, 200), (int(meanG[2]),int(meanG[1]),int(meanG[0])))
    # meanG.show()
    #
    # cv2.namedWindow(f'Image ', cv2.WINDOW_GUI_NORMAL)
    # cv2.resizeWindow(f'Image ', 1000, 800)
    # cv2.imshow(f'Image ', img)
    #
    # Wait longer to prevent freeze for videos.
    # if cv2.waitKey(60000) & 0xFF == ord('q'):
    #   return stat

    return stat


def grey_edge(image, njet=0, mink_norm=1, sigma=1):
    """
    Estimates the light source of an input_image as proposed in:
    J. van de Weijer, Th. Gevers, A. Gijsenij
    "Edge-Based Color Constancy"
    IEEE Trans. Image Processing, accepted 2007.
    Depending on the parameters the estimation is equal to Grey-World, Max-RGB, general Grey-World,
    Shades-of-Grey or Grey-Edge algorithm.
    :param image: rgb input image (NxMx3)
    :param njet: the order of differentiation (range from 0-2)
    :param mink_norm: minkowski norm used (if mink_norm==-1 then the max
           operation is applied which is equal to minkowski_norm=infinity).
    :param sigma: sigma used for gaussian pre-processing of input image
    :return: illuminant color estimation
    :raise: ValueError
    """

    # pre-process image by applying gauss filter
    gauss_image = filters.gaussian(image, sigma=sigma, multichannel=True)

    # get njet-order derivative of the pre-processed image
    if njet == 0:
        deriv_image = [gauss_image[:, :, channel] for channel in range(3)]
    else:
        if njet == 1:
            deriv_filter = filters.sobel
        elif njet == 2:
            deriv_filter = filters.laplace
        else:
            raise ValueError("njet should be in range[0-2]! Given value is: " + str(njet))
        deriv_image = [np.abs(deriv_filter(gauss_image[:, :, channel])) for channel in range(3)]

    # remove saturated pixels in input image
    for channel in range(3):
        deriv_image[channel][image[:, :, channel] >= 255] = 0.

    # estimate illuminations
    if mink_norm == -1:  # mink_norm = inf
        estimating_func = np.max
    else:
        estimating_func = lambda x: np.power(np.sum(np.power(x, mink_norm)), 1 / mink_norm)
    illum = [estimating_func(channel) for channel in deriv_image]

    # normalize estimated illumination
    som = np.sqrt(np.sum(np.power(illum, 2)))
    illum = np.divide(illum, som)

    return np.abs(illum[0]-illum[1])+np.abs(illum[0]-illum[2])+np.abs(illum[1]-illum[2])


def train(args):
    np.random.seed(args.seed)
    torch.manual_seed(args.seed)
    torch.cuda.manual_seed(args.seed)

    device = torch.device('cuda' if args.cuda and torch.cuda.is_available() else 'cpu')

    transform = transforms.Compose([
        transforms.Resize(args.image_size),
        transforms.CenterCrop(args.image_size),
        # utils.RGB2LAB(),
        transforms.ToTensor(),
        # utils.LAB2Tensor(),
    ])
    pert_transform = transforms.Compose([
        utils.PerturbWhiteBalance(),
    #    utils.PerturbBrightness(),
    ])
    trainset = utils.FlatImageFolder(args.dataset, transform, pert_transform)
    trainloader = DataLoader(trainset, batch_size=args.batch_size, shuffle=True, pin_memory=True, num_workers=4)
    model = TransformerNet()
#    if args.gpus is not None:
#        model = nn.DataParallel(model, device_ids=args.gpus)
#    else:
#        model = nn.DataParallel(model)
    if args.resume:
        state_dict = torch.load(args.resume)
        model.load_state_dict(state_dict)

    if args.cuda:
        model.cuda()

    optimizer = torch.optim.Adam(model.parameters(), args.lr, eps=0.000001)
    criterion = nn.BCELoss()

    # criterion = nn.MSELoss()
    # criterion = nn.SmoothL1Loss()
    # criterion = nn.CosineEmbeddingLoss(reduction='none')
    # criterion = nn.MultiMarginLoss()

    start_time = datetime.now()

    for e in range(args.epochs):
        model.train()
        count = 0
        loss = 0.0
        acc_loss = 0.0
        for batchi, (pert_img, ori_img) in enumerate(trainloader):
            count += len(pert_img)
            if args.cuda:
                pert_img = pert_img.cuda(non_blocking=True)
                ori_img = ori_img.cuda(non_blocking=True)

            def closure():
                optimizer.zero_grad()
                rec_img = model(pert_img)
                loss = criterion(rec_img, ori_img)
                loss.backward()
                return loss.item()
            acc_loss += optimizer.step(closure)
            # acc_loss += loss.item()

            if (batchi + 1) % args.log_interval == 0:
                mesg = '{}\tEpoch {}: [{}/{}]\ttotal loss: {:.6f}'.format(
                    time.ctime(), e + 1, count, len(trainset), acc_loss/(args.log_interval))
                print(mesg)
                acc_loss = 0.0

        if args.checkpoint_dir and e + 1 != args.epochs:
            model.eval().cpu()
            ckpt_filename = 'ckpt_epoch_' + str(e+1) + '.pth'
            ckpt_path = osp.join(args.checkpoint_dir, ckpt_filename)
            torch.save(model.state_dict(), ckpt_path)
            model.cuda().train()
            print('Checkpoint model at epoch %d saved' % (e+1))

    model.eval().cpu()
    if args.save_model_name:
        model_filename = args.save_model_name
    else:
        model_filename = "epoch_" + str(args.epochs) + "_" + str(time.ctime()).replace(' ', '_') + ".model"
    model_path = osp.join(args.save_model_dir, model_filename)
    torch.save(model.state_dict(), model_path)

    compiled_model = torch.jit.script(model)
    torch.jit.save(compiled_model, 'model/jit_matsumoto.pt')

    end_time = datetime.now()

    print('Finished training after %s, trained model saved at %s' % (end_time - start_time , model_path))


def check_path(path):
    try:
        if not osp.exists(path):
            os.makedirs(path)
    except OSError as e:
        print(e)
        sys.exit(1)


def str2bool(v):
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


def image_stats(image):
    (l, a, b) = cv2.split(image)
    (lMean, lStd) = (l.mean(), l.std())
    (aMean, aStd) = (a.mean(), a.std())
    (bMean, bStd) = (b.mean(), b.std())

    return (lMean, lStd, aMean, aStd, bMean, bStd)


def color_transfer(s, t):

    s = cv2.cvtColor(s, cv2.COLOR_BGR2LAB).astype("float32")
    t = cv2.cvtColor(t, cv2.COLOR_BGR2LAB).astype("float32")

    (lMeanSrc, lStdSrc, aMeanSrc, aStdSrc, bMeanSrc, bStdSrc) = image_stats(s)
    (lMeanTar, lStdTar, aMeanTar, aStdTar, bMeanTar, bStdTar) = image_stats(t)

    # subtract the means from the target image
    (l, a, b) = cv2.split(t)

    l -= lMeanTar
    a -= aMeanTar
    b -= bMeanTar

    # l = l * (lStdSrc / lStdTar)
    # a = a * (aStdSrc / aStdTar)
    # b = b * (bStdSrc / bStdTar)

    # l += lStdSrc/lStdTar
    # a += aStdSrc/aStdTar
    # b += bStdSrc/bStdTar

    l += lMeanSrc
    a += aMeanSrc
    b += bMeanSrc

    l = np.clip(l, 0, 255)
    a = np.clip(a, 0, 255)
    b = np.clip(b, 0, 255)

    transfer = cv2.merge([l, a, b])
    transfer = cv2.cvtColor(transfer.astype("uint8"), cv2.COLOR_LAB2BGR)

    return transfer


def adjust_saturation(image, q):

    #q = ((255-q)/255)    
    q = math.sqrt(255 - q)
    print('q : ' +  str(q))

    hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV_FULL)

    # skin mask 
    # skin_lower = np.array([5,22,100])
    # skin_upper = np.array([25,155,255])
    # skin_mask = cv2.inRange(hsv, skin_lower, skin_upper)
    # skin_mask = cv2.GaussianBlur(skin_mask, (27, 27), 9)
    # skin_hsv = cv2.bitwise_and(hsv, hsv, mask=skin_mask)

    image_32 = image.astype(np.float32)
    hsv_32 = cv2.cvtColor(image_32, cv2.COLOR_BGR2HSV_FULL)
    h, s, v = cv2.split(hsv_32)
    s = s + (s/np.mean(v))*q
    hsv = cv2.merge([h, s, v])

    return cv2.cvtColor(hsv, cv2.COLOR_HSV2BGR_FULL)


def gamma_correct(image, v):


    # hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV_FULL)
    # hue, sat, val = cv2.split(hsv)
    # mean = np.mean(val)

    # gamma = math.sqrt(v/255)*np.sat
    gamma = math.sqrt(v/255)
    print('gamma value : ' + str(gamma))

    table = np.array([((i / 255.0) ** gamma) * 255
        for i in np.arange(0, 256)]).astype("uint8")

    return cv2.LUT(image.astype(np.uint8), table)


def clahe_gamma_correct(img, v):

    lab = cv2.split(img.astype(np.uint8))
    gray = cv2.cvtColor(img.astype(np.uint8), cv2.COLOR_BGR2GRAY)
    clip_limit = 1.0
    clahe = cv2.createCLAHE(clipLimit=clip_limit, tileGridSize=(16, 16))
    equalized = clahe.apply(gray)

    eq = []
    for ch, color in zip(lab, ['B', 'G', 'R']):
        eq.append(clahe.apply(ch))

    eq = cv2.merge(eq)

    return eq


def full_gamma_correct(img, v):

    gamma = math.sqrt(((1 + math.sin((255-v)/255))/2))

    lab = cv2.split(img.astype(np.uint8))
    gray = cv2.cvtColor(img.astype(np.uint8), cv2.COLOR_BGR2GRAY)
    clip_limit = 1 - v/256
    clahe = cv2.createCLAHE(clipLimit=clip_limit, tileGridSize=(8, 8))

    #se=cv2.getStructuringElement(cv2.MORPH_RECT , (32,32))
    #bg=cv2.morphologyEx(gray, cv2.MORPH_DILATE, se)
    #out_gray=cv2.divide(gray, bg, scale=255)

    equalized = clahe.apply(gray)

    eq = []
    for ch, color in zip(lab, ['B', 'G', 'R']):
        eq.append(clahe.apply(ch))

    eq = cv2.merge(eq).astype(np.float32)

    print('gamma ' + str(gamma))

    table = np.array([((i / 255.0) ** (1/gamma)) * 255
        for i in np.arange(0, 256)]).astype("uint8")

    return cv2.LUT(eq.astype(np.uint8), table)


def evaluate(args):

    device = torch.device('cuda' if args.cuda and torch.cuda.is_available() else 'cuda')
    #device = 'cpu'

    brightness_lower_threshold = 125;
    brightness_higher_threshold = 175;
    saturation_threshold = 100;

    # Infef if white balance color is correct or not
    if args.prod is not None:
        HasWhiteBalanceModel = torch.jit.load('model/jit_color.pt')
    else:
        HasWhiteBalanceModel = torch.load('model/color.mdl')

    net = resnet18(pretrained=True)
    net.fc = nn.Sequential(nn.Linear(512, 1000), nn.ReLU(), nn.Linear(1000, 1000), nn.ReLU(), nn.Linear(1000, 100),
                           nn.ReLU(), nn.Linear(100, 1), nn.Sigmoid())

    net.load_state_dict(HasWhiteBalanceModel["state_dict"])
    net.eval()
    net.to(device)
    # print(net)


    # load model to correct white balance
    WhiteBalanceModel = TransformerNet()
    if args.prod is not None:
        state_dict = torch.jit.load('model/jit_matsumoto.pt')
    else:
        state_dict = torch.load(args.model)

    if args.gpus is not None:
        WhiteBalanceModel = nn.DataParallel(WhiteBalanceModel, device_ids=args.gpus)
    #    HasWhiteBalanceModel = nn.DataParallel(HasWhiteBalanceModel, device_ids=args.gpus)
    else:
        WhiteBalanceModel = nn.DataParallel(WhiteBalanceModel)
    #    HasWhiteBalanceModel = nn.DataParallel(HasWhiteBalanceModel)

    WhiteBalanceModel.load_state_dict(state_dict)
    #HasWhiteBalanceModel.load_state_dict("state_dict")

    #if args.cuda:
    #    WhiteBalanceModel.cuda()
    #    HasWhiteBalanceModel.cuda()
    WhiteBalanceModel.cuda()

    with torch.no_grad():
        for root, dirs, filenames in os.walk(args.input_dir):
            outdir = os.path.join(args.output_dir, os.path.relpath(root, args.input_dir))
            if not os.path.isdir(outdir):
                os.mkdir(outdir)
            for filename in filenames:
                if utils.is_image_file(filename):
                    print(" - - - " + filename + " - - - ")


                    impath = osp.join(root, filename)
                    img = utils.load_image(impath)
                    img_stats = analyse(img)

                    origin = cv2.imread(impath)

                    # score the need for color balance
                    newsize = (720, 720)
                    simg = img.resize(newsize)
                    #simg = img.convert('RGB')

                    #simg_tensor = transforms.ToTensor()(simg)
                    #simg_tensor = simg_tensor.unsqueeze(0)
                    #if args.cuda:
                    #    simg_tensor.cuda()
                    #    score = WhiteBalanceModel(simg_tensor)
                    #else:
                    #    simg_tensor.cpu()

                    trans1 = transforms.ToTensor()
                    # plt.imshow(img)
                    # plt.show()
                    data=trans1(simg).to(device)
                    score=net(data.unsqueeze(0))

                    output = full_gamma_correct(origin, img_stats['brightness'])

                    print("color correction score : " + str(score))
                    if (score < 0.5):
                        print('noCorrection')
                        output = full_gamma_correct(origin, img_stats['brightness'])
                    else:
                        print('doCorrection')
                        # Apply Deep Learning Transfer
                        if (img_stats['white_balance'] > 0.6):
                            print('deep learning white balance')
                            img_tensor = transforms.ToTensor()(img)
                            img_tensor = img_tensor.unsqueeze(0)
                            print(img_tensor.device)
                            rec_img = WhiteBalanceModel(img_tensor)

                            # utils.save_image(rec_img[0], save_path)
                            dl_img = utils.save_image_preserv_length(rec_img[0], img_tensor[0], '.')
                            output = color_transfer(dl_img, output)

                    #    else:
                    #        print('simple white balance')
                    #        output = simplest_cb(origin, 1, True)

                    # debug code , delete
                    # x_output = simplest_cb(origin, 1, True)
                    # x_filename =  'SCB_' + filename
                    # x_save_path = osp.join(outdir, x_filename)
                    # cv2.imwrite(x_save_path,x_output)

                    # Apply Clahe Gamma Correction
                    # if (img_stats['brightness'] > brightness_higher_threshold or img_stats['brightness'] < brightness_lower_threshold):
                    #    filename =  'CGC_' + filename
                    #    output = clahe_gamma_correct(origin, img_stats['brightness'])
                    # else:
                    #    output = origin
                    # output = clahe_gamma_correct(origin, img_stats['brightness'])
                    # output = gamma_correct(origin, img_stats['brightness'])
                    # output = full_gamma_correct(origin, img_stats['brightness'])

                    # Apply Deep Learning Transfer
                    # if (img_stats['white_balance'] > 0.9):
                    #    img_tensor = transforms.ToTensor()(img)
                    #    img_tensor = img_tensor.unsqueeze(0)
                    #    if args.cuda:
                    #        img_tensor.cuda()
                    #        rec_img = model(img_tensor)
                    #    if args.cuda:
                    #        rec_img = rec_img.cpu()
                    #        img_tensor = img_tensor.cpu()

                    # utils.save_image(rec_img[0], save_path)
                    #    dl_img = utils.save_image_preserv_length(rec_img[0], img_tensor[0], save_path)
                    #    output = color_transfer(dl_img, output)
                    #    filename =  'DLT_' + filename

                    # Apply Simplest Color Balance
                    # elif (img_stats['white_balance'] > 0.2):

                    # if (img_stats['white_balance'] > 0.5):
                    #     output = simplest_cb(output, 1, True)
                    #     filename =  'SCB_' + filename


                    # finally adjust increase saturation if too low.
                    #if (img_stats['saturation'] < saturation_threshold):
                    #    output = adjust_saturation(output, 50-img_stats['saturation'])
                    #    filename =  'SA_' + filename

                    # TODO : reapply saturation
                    output = adjust_saturation(output, img_stats['saturation'])

                    save_path = osp.join(outdir, filename)
                    print('save_path')
                    print(save_path)
                    cv2.imwrite(save_path,output)


def eval_service(img, net, WhiteBalanceModel):

    brightness_lower_threshold = 125;
    brightness_higher_threshold = 175;
    saturation_threshold = 100;

    with torch.no_grad():

        img_stats = analyse(img)

        #origin = cv2.imread(img)
        origin = np.array(img) 
        origin = origin[:, :, ::-1].copy()

        # score the need for color balance
        newsize = (720, 720)
        simg = img.resize(newsize)
        #simg = img.convert('RGB')

        #simg_tensor = transforms.ToTensor()(simg)
        #simg_tensor = simg_tensor.unsqueeze(0)
        #if args.cuda:
        #    simg_tensor.cuda()
        #    score = WhiteBalanceModel(simg_tensor)
        #else:
        #    simg_tensor.cpu()

        trans1 = transforms.ToTensor()
        # plt.imshow(img)
        # plt.show()
        data=trans1(simg).to('cpu')
        score=net(data.unsqueeze(0))

        output = full_gamma_correct(origin, img_stats['brightness'])

        print("color correction score : " + str(score))
        if (score < 0.5):
            print('noCorrection')
            output = full_gamma_correct(origin, img_stats['brightness'])
        else:
            print('doCorrection')
            # Apply Deep Learning Transfer
            if (img_stats['white_balance'] > 0.6):
                print('deep learning white balance')
                img_tensor = transforms.ToTensor()(img)
                img_tensor = img_tensor.unsqueeze(0)
                print(img_tensor.device)
                rec_img = WhiteBalanceModel(img_tensor)

                # utils.save_image(rec_img[0], save_path)
                dl_img = utils.save_image_preserv_length(rec_img[0], img_tensor[0], '.')
                output = color_transfer(dl_img, output)

        output = adjust_saturation(output, img_stats['saturation'])

        is_success, buffer = cv2.imencode(".jpg", output)

        return buffer


def apply_mask(matrix, mask, fill_value):
    masked = np.ma.array(matrix, mask=mask, fill_value=fill_value)
    return masked.filled()


def apply_threshold(matrix, low_value, high_value):
    low_mask = matrix < low_value
    matrix = apply_mask(matrix, low_mask, low_value)

    high_mask = matrix > high_value
    matrix = apply_mask(matrix, high_mask, high_value)

    return matrix


def simplest_cb(img, percent, perturb):
    assert img.shape[2] == 3
    assert percent > 0 and percent < 100

    half_percent = percent / 200.0

    channels = cv2.split(img)

    out_channels = []
    for channel in channels:
        assert len(channel.shape) == 2
        # find the low and high precentile values (based on the input percentile)
        height, width = channel.shape
        vec_size = width * height
        flat = channel.reshape(vec_size)

        assert len(flat.shape) == 1

        flat = np.sort(flat)

        n_cols = flat.shape[0]

        low_val  = flat[math.floor(n_cols * half_percent)]
        high_val = flat[math.ceil( n_cols * (1.0 - half_percent))]

        # print("Lowval: ", low_val)
        # print("Highval: ", high_val)

        # saturate below the low percentile and above the high percentile
        thresholded = apply_threshold(channel, low_val, high_val)
        # scale the channel
        normalized = cv2.normalize(thresholded, thresholded.copy(), 0, 255, cv2.NORM_MINMAX)
        out_channels.append(normalized)

    return cv2.merge(out_channels)


def main():
    main_arg_parser = argparse.ArgumentParser()
    subparsers = main_arg_parser.add_subparsers(title='subcommands', dest='subcommand')

    train_parser = subparsers.add_parser('train', help='train the network')
    train_parser.add_argument('--epochs', type=int, default=2, help='number of training epochs, default is 2')
    train_parser.add_argument('--batch-size', type=int, default=30, help='training batch size, default is 30')
    train_parser.add_argument('--dataset', required=True, help='path to training dataset, the path should '
                                'point to a folder containing another folder with all the training images')
    train_parser.add_argument('--save-model-dir', default='model', help='directory of the model to be saved, default is model/')
    train_parser.add_argument('--save-model-name', default=None, help='save model name')
    train_parser.add_argument('--image-size', type=int, default=256, help='size of training images, default is 256')
    train_parser.add_argument('--cuda', action='store_true', default=False, help='run on GPU')
    train_parser.add_argument('--seed', type=int, default=42, help='random seed for training')
    train_parser.add_argument('--lr', type=float, default=1e-3, help='learning rate, default is 0.001')

    train_parser.add_argument('--log-interval', type=int, default=100, help='number of images after which the training loss is logged,'
                                                                            ' default is 100')
    train_parser.add_argument('--checkpoint-dir', default=None, help='checkpoint model saving directory')
    train_parser.add_argument('--resume', default=None, help='resume training from saved model')
    train_parser.add_argument('--gpus', type=int, nargs='*', default=None, help='specify GPUs to use')

    eval_parser = subparsers.add_parser('eval', help='eval the network')
    eval_parser.add_argument('--input-dir', required=True, help='path to input image directory')
    eval_parser.add_argument('--output-dir', default='output', help='path to output image directory')
    eval_parser.add_argument('--model', required=True, help='saved model to be used for evaluation')
    eval_parser.add_argument('--cuda', action='store_true', default=False, help='run on GPU')
    eval_parser.add_argument('--gpus', type=int, nargs='*', default=None, help='specify GPUs to use')
    eval_parser.add_argument('--prod', action='store_true', default=False, help='run production torchscript models')
    args = main_arg_parser.parse_args()

    if args.subcommand is None:
        print('ERROR: specify either train or eval')
        sys.exit(1)
    if args.cuda and not torch.cuda.is_available():
        print("ERROR: cuda is not available, try running on CPU")
        sys.exit(1)
    if args.subcommand == 'train':
        check_path(args.save_model_dir)
        if args.checkpoint_dir:
            check_path(args.checkpoint_dir)
        train(args)
    else:
        check_path(args.output_dir)
        evaluate(args)


if __name__ == '__main__':
    main()

