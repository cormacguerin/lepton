import csv 
import sys
import json 
  
  
# Function to convert a CSV to JSON 
# Takes the file paths as arguments 
def make_json(csvFilePath): 
      
    # create a dictionary 
    data = []
      
    # Open a csv reader called DictReader 
    with open(csvFilePath, encoding='utf-8') as csvf: 
        csvReader = csv.DictReader(csvf) 
          
        # Convert each row into a dictionary  
        # and add it to data 
        for rows in csvReader: 
            data.append(rows)
  
    print(json.dumps(data, indent=4))
          
# Driver Code 
  
# Decide the two file paths according to your  
# computer system 
csvFilePath = sys.argv[1]
  
# Call the make_json function 
make_json(csvFilePath)
