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
def get_trends_available_woeids():
    woeids = []
    results = api.trends_available()

    for r in results:
        if (r['country'] == 'United States') or (r['country'] == 'United Kingdom') or (r['country'] == 'Australia') or (r['country'] == 'New Zealand') or (r['country'] == 'Ireland'):
            woeids.append((r['woeid'],r['country']))

    return woeids


def get_trending_hashtags():
    hashtags = []
    for k,v in get_trends_available_woeids():
        response = api.trends_place(k)
        for h in response[0]['trends']:
            time.sleep(2)
            hashtags.append(urllib.parse.unquote(h['query']))

    return set(hashtags)


def get_tweets(q):
    #tweets = api.user_timeline(screen_name='elonmusk', count=50, exclude_replies=True).items(1000)
    #for tweet in tw.Cursor(api.user_timeline, screen_name = 'elonmusk').items(1000):
    for x in api.search(q, tweet_mode='extended'):        
        return {
            'id':x._json['id'],
            'full_text':x._json['full_text'],
            'data':x._json['created_at'],
            'hashtags':' '.join(str(h) for h in x._json['entities']['hashtags']),
            'user_name':x._json['user']['name'],
            'user_screen_name':x._json['user']['screen_name'],
            'user_followers_count':x._json['user']['followers_count'],
            'lang':x._json['lang'],
            'favorite_count':x._json['favorite_count'],
            'retweet_count':x._json['retweet_count'],
            'favorited':x._json['favorited'],
            'retweeted':x._json['retweeted']
        }


#tweets = tw.Cursor(api.search,
#                       q=search_words,
#                       lang="en",
#                       since=date_since).items(5)



if __name__ == "__main__":
    #get_tweets()
    for q in get_trending_hashtags():
        print(get_tweets(q))



# Collect a list of tweets
#[tweet.text for tweet in tweets]

#print(api.VerifyCredentials())

#statuses = api.GetUserTimeline(screen_name='elonmusk')
#print(statuses)
#print([s.id for s in statuses])


