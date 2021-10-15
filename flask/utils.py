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


from torch.utils.data import Dataset
from torchvision import transforms

from skimage import io, color
from PIL import Image, ImageEnhance, ImageStat
import os.path as osp
import os
import torch
from torch import nn
import numpy as np
import math
import cv2

IMG_EXTENSIONS = [
    '.jpg', '.JPG', '.jpeg', '.JPEG',
    '.png', '.PNG', '.ppm', '.PPM', '.bmp', '.BMP',
]


def is_image_file(filename):
    return any(filename.endswith(extension) for extension in IMG_EXTENSIONS)


def load_image(path):
    img = Image.open(path).convert('RGB')
    # img = transforms.ToTensor()(img)
    # img = io.imread(path)
    # img = img.transpose(2, 0, 1)
    # img = torch.from_numpy(img).float()
    return img


def save_image(tensor, dir):
    if tensor.max() > 1:
        tensor = tensor / tensor.max()
    img = tensor.clone().mul(255).clamp(0, 255).numpy()
    img = img.transpose(1, 2, 0).astype('uint8')
    # io.imsave(dir, img)
    return img

def save_image_preserv_length(tensor, ori, dir):
    tensor = normalize(tensor, dim=0)

    orilen = ori.clone() ** 2
    orilen = orilen.sum(dim=0).sqrt().unsqueeze(0)

    if tensor.max() > 1:
        tensor = tensor / tensor.max()

    # tenlen = torch.zeros(tensor.size()[0], tensor.size()[1], tensor.size()[2]).cuda()
    tenlen = torch.zeros(tensor.size()[0], tensor.size()[1], tensor.size()[2]).cpu()
    tenlen[:, :orilen.size()[1], :orilen.size()[2]] = orilen
    tensor = tensor * tenlen

    img = tensor.cpu().clone().mul(255).clamp(0, 255).numpy()
    img = img.transpose(1, 2, 0).astype('uint8')
    # io.imsave(dir, img)
    return img

def save_labimage(tensor, dir):
    img = tensor.clone().mul(255).clamp(0, 255).numpy()
    img = img.transpose(1, 2, 0).astype('uint8')
    img = cv2.cvtColor(img, cv2.COLOR_Lab2RGB)
    io.imsave(dir, img)

def gram_matrix(y):
    (b, ch, h, w) = y.size()
    features = y.view(b, ch, w * h)
    features_t = features.transpose(1, 2)
    gram = features.bmm(features_t) / (ch * h * w)
    return gram

def iter_dir(dir):
    images = []
    for root, dirs, filenames in os.walk(dir):
        for filename in filenames:
            if is_image_file(filename):
                path = osp.join(root, filename)
                images.append(path)
    return images

class RGB2LAB():
    def __call__(self, img):
        npimg = np.array(img)
        img = cv2.cvtColor(npimg, cv2.COLOR_RGB2Lab)
        pilimg = Image.fromarray(img)
        return pilimg

def Saturate(img, s):
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    pil_img = Image.fromarray(img)
    filter = ImageEnhance.Color(pil_img)
    pil_img = filter.enhance(s).convert('RGB')
    open_cv_image = np.array(pil_img)
    return open_cv_image[:, :, ::-1].copy() 

def WhiteBalance(img):
    result = cv2.cvtColor(img, cv2.COLOR_BGR2LAB)
    avg_a = np.average(result[:, :, 1])
    avg_b = np.average(result[:, :, 2])
    result[:, :, 1] = result[:, :, 1] - ((avg_a - 128) * (result[:, :, 0] / 255.0) * 1.1)
    result[:, :, 2] = result[:, :, 2] - ((avg_b - 128) * (result[:, :, 0] / 255.0) * 1.1)
    result = cv2.cvtColor(result, cv2.COLOR_LAB2BGR)
    return result

def WhiteDeBalance(img):
    perturb = np.random.uniform(-1.0, 1.0)
    result = cv2.cvtColor(img, cv2.COLOR_BGR2LAB)
    avg_a = np.average(result[:, :, 1])
    avg_b = np.average(result[:, :, 2])
    L = np.sqrt(sum([0.299*(result[0][0]**2),0.587*(result[0][1]**2),0.114*(result[0][2]**2)]))

    result[:, :, 2] = result[:, :, 2] - avg_b * perturb * L[0] / 255.0
    result[:, :, 0] = result[:, :, 0] - avg_c * perturb * L[0] / 255.0

    result = cv2.cvtColor(result, cv2.COLOR_LAB2BGR)
    return result

def normalize(tensor, dim):
    tensor = tensor.clamp(1e-10)
    tensorlen = (tensor.clone() ** 2).sum(dim=dim).sqrt().unsqueeze(dim)
    # tensorlen[tensorlen==0] = 1
    tensor = tensor / tensorlen
    return tensor

class AngularLoss(nn.Module):
    def __init__(self, cuda):
        super(AngularLoss, self).__init__()
        self.one = torch.tensor(1, dtype=torch.float)
        if cuda:
            self.one = self.one.cuda()

    def normalize(self, tensor, dim):
        tensor = transforms.Normalize(2, 0.5)(tensor)
        #tensor = tensor.clamp(1e-10)
        #tensorlen = (tensor.clone() ** 2).sum(dim=dim).sqrt().unsqueeze(dim)
        #tensor = tensor / tensorlen
        return tensor

    def forward(self, input, target):
        batchsize, _, w, h = input.size()
        input = self.normalize(input, dim=1)
        target = self.normalize(target, dim=1)
        loss = input.mul(target).sum(dim=1).mean()
        loss = self.one - loss
        return loss

class MaeLoss():

    @staticmethod
    def compute(output, target):
        loss = torch.sum(torch.abs(output - target)) / output.size(0)
        return loss

def GenRand(f):
    if np.random.uniform(-1, 1) > 0:
        return np.random.uniform(1-f, 1-f/2)
    else:
        return np.random.uniform(1+f/2, 1+f)

class PerturbBrightness():

    def __call__(self, tensor_img):
        img = tensor_img.numpy()

        img[0, :, :] = (img[0, :, :] +0.5)/2
        img[1, :, :] = (img[1, :, :] +0.5)/2
        img[2, :, :] = (img[2, :, :] +0.5)/2

        img = torch.from_numpy(img)
        if img.max() > 1:
            img = img / img.max()
        
        avg_a = np.average(img[0, :, :])
        # perturbed
        img_p = img.clone().mul(255).clamp(0, 255).numpy()
        img_p = img_p.transpose(1, 2, 0).astype('uint8')
        io.imsave('gen_p/' + str(avg_a) + '.jpg', img_p)

        return img


class PerturbWhiteBalance():

    def __call__(self, tensor_img):
        img = tensor_img.numpy()

        perturb_a = GenRand(0.5)
        perturb_b = GenRand(0.5)
        perturb_c = GenRand(0.5)

        avg_a = np.average(img[0, :, :])
        avg_b = np.average(img[1, :, :])
        avg_c = np.average(img[2, :, :])

        # luminance estimation
        L = np.sqrt(sum([0.299*(img[0]**2),0.587*(img[1]**2),0.114*(img[2]**2)]))

        # original
        img_ = torch.from_numpy(img)
        if img_.max() > 1:
            img_ = img_ / img_.max()

        img_o = torch.from_numpy(img)
        img_o = img_o.clone().mul(255).clamp(0, 255).numpy()
        img_o = img_o.transpose(1, 2, 0).astype('uint8')
        # save generated perturbed images for debugging
        io.imsave('gen_o/' + str(avg_a) + '.jpg', img_o)

        mod = False

        if np.random.uniform(-1, 1) > 0:
            img[0, :, :] = img[0, :, :] - avg_a * perturb_a * L / 2
            mod = True

        if np.random.uniform(-1, 1) > 0:
            img[1, :, :] = img[1, :, :] - avg_b * perturb_b * L / 2
            mod = True

        if np.random.uniform(-1, 1) > 0:
            img[2, :, :] = img[2, :, :] - avg_c * perturb_c * L / 2
            mod = True

        if mod == False:
            img[0, :, :] = img[0, :, :] - avg_a * perturb_a * L / 2
            img[1, :, :] = img[1, :, :] - avg_b * perturb_b * L / 2
            img[2, :, :] = img[2, :, :] - avg_c * perturb_c * L / 2

        # Add Contrast (testing contrast correction / mixed results)
        #
        # if np.random.uniform(-1, 1) > 1.0:
        #    _a = GenRand(0.8)
        #    img[0, :, :] = img[0, :, :] + _a
        #    img[1, :, :] = img[1, :, :] + _a
        #    img[2, :, :] = img[2, :, :] + _a

        img = torch.from_numpy(img)
        if img.max() > 1:
            img = img / img.max()
        
        # perturbed
        img_p = img.clone().mul(255).clamp(0, 255).numpy()
        img_p = img_p.transpose(1, 2, 0).astype('uint8')
        io.imsave('gen_p/' + str(avg_a) + '.jpg', img_p)

        return img


class FlatImageFolder(Dataset):
    def __init__(self, root, transform=None, pert_transform=None):
        self.imgs = iter_dir(root)
        if len(self.imgs) == 0:
            raise (RuntimeError("Found 0 images in folders of: " + root + "\n"))
        self.root = root
        self.transform = transform
        self.pert_transform = pert_transform

    def __len__(self):
        return len(self.imgs)

    def __getitem__(self, idx):
        path = self.imgs[idx]
        img = Image.open(path).convert('RGB')
        if self.transform is not None:
            img = self.transform(img)

        perturb = img.clone()

        if self.pert_transform:
            perturb = self.pert_transform(perturb)

        return perturb, img
