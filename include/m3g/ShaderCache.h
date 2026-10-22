#pragma once
template<class K,class V> class hash_table {
public: hash_table(); ~hash_table(); unsigned size() const; V& operator[](const K& key);
};
