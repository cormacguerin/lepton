#include <map>
#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_


class Functions {

	Functions::Functions()
	{
	}

	Functions::~Functions()
	{
	}

	public:

	template<typename KeyType, typename LeftValue, typename RightValue>
		map<KeyType, pair<LeftValue, RightValue> > IntersectMaps(const map<KeyType, LeftValue> & left, const map<KeyType, RightValue> & right)
		{
			map<KeyType, pair<LeftValue, RightValue> > result;
			typename map<KeyType, LeftValue>::const_iterator il = left.begin();
			typename map<KeyType, RightValue>::const_iterator ir = right.begin();
			while (il != left.end() && ir != right.end())
			{
				if (il->first < ir->first)
					++il;
				else if (ir->first < il->first)
					++ir;
				else
				{
					result.insert(make_pair(il->first, make_pair(il->second, ir->second)));
					++il;
					++ir;
				}
			}
			return result;
		}

};

#endif
