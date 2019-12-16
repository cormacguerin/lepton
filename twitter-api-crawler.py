# https://github.com/bear/python-twitter
# pip install python-twitter 
 
import sys
import json
import time
import logging
import twitter
import urllib.parse
import tweepy as tw
import pandas as pd

consumer_key='DH4707YVei7fw3SiiK61yCBir'
consumer_secret='EC6I92ncLEXK43ExyHPI2Z4YbHAnWljgxZQ9eJ4Cf1przHhrCZ'
access_token='787153039-BudbfF96NEcURZ7ctMBuh6CUPbbbUzwyd9Lvyp5W'
access_token_secret='wCbKohaQJRfH3iL9Ajr80PJnizQ9mFpqnBqAIhsSdsODO'

auth = tw.OAuthHandler(consumer_key, consumer_secret)
auth.set_access_token(access_token, access_token_secret)
api = tw.API(auth, wait_on_rate_limit=True)

#search_words = "#wildfires"
#date_since = "2018-11-16"

def get_tweets():
    #tweets = api.user_timeline(screen_name='elonmusk', count=50, exclude_replies=True).items(1000)
    for tweet in tw.Cursor(api.user_timeline, screen_name = 'elonmusk').items(1000):
        print(tweet)
    yield tweet
        #yield [s.id for s in statuses]



#tweets = tw.Cursor(api.search,
#                       q=search_words,
#                       lang="en",
#                       since=date_since).items(5)
if __name__ == "__main__":
    for tweet in get_tweets():
        print(tweet)
        #for reply in get_replies(tweet):
        #    print(reply.AsJsonString())


# Collect a list of tweets
#[tweet.text for tweet in tweets]

#print(api.VerifyCredentials())

#statuses = api.GetUserTimeline(screen_name='elonmusk')
#print(statuses)
#print([s.id for s in statuses])


