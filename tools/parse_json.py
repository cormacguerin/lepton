import json

f = open('working_flipkart.json',)

jsonObject = json.load(f)

for key in jsonObject:
    if ('metadata' in key.keys()):
        key['metadata']['price'] = key['discounted_price']
        key['metadata']['brand'] = key['brand']

print(json.dumps(jsonObject))

