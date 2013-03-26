import re

class Parameters(dict):
    def __init__(self, *args, **kwargs):
        super(Parameters, self).__init__(*args, **kwargs)
        self.__separator = '.'
    
    def _set_separator(self, separator):
        self.__separator = separator
    
    def _myparams(self, keys, path):
        ''' From the hierarchical dictionary params, generate a flat 
        dictionary with those parameters which are listed in keys and that 
        are found along path
        
        >>> Parameters({'a':1, 'b':2, 'foo':{'a':3}, 'bar':{'a':4}}\
        )._myparams(keys=("a", "b"), path = ("foo",))
        {'a': 3, 'b': 2}
        '''
        result = {}
        source = self
        idx = 0
        while source:
          tomerge = {key:source[key] for key in keys 
                     if key in source and not isinstance(source[key], dict)}
          result.update(tomerge)
          if path and idx < len(path) and path[idx] in source:
            source = source[path[idx]]
            idx = idx + 1
          else:
            source = None
        return result

    def _insertkey(self, keypath, value):
        ''' key is a path with segments delimited by the separator, value is 
        inserted in the hierarchical parameters dictionary at the location 
        specified by path
        Keys overwrite previously existing keys, but a conflict between a key
        and a sub-dictionary causes a KeyError (similar to open('filepath','w')
        when filepath exists as a subdirectory).
        
        >>> p=Parameters({'a':1, 'b':2, 'foo':{'a':3}, 'bar':{'a':4}})
        >>> p._insertkey('c', 3)
        >>> p
        {'a': 1, 'b': 2, 'c': 3, 'bar': {'a': 4}, 'foo': {'a': 3}}
        >>> p._insertkey('bar.c', 5)
        >>> p
        {'a': 1, 'b': 2, 'c': 3, 'bar': {'a': 4, 'c': 5}, 'foo': {'a': 3}}
        >>> p._insertkey('bar.baz.c', 6)
        >>> p
        {'a': 1, 'b': 2, 'c': 3, 'bar': {'a': 4, 'c': 5, 'baz': {'c': 6}}, 'foo': {'a': 3}}
        >>> p._insertkey('bar.baz.c.d', 6)
        Traceback (most recent call last):
        TypeError: argument of type 'int' is not iterable
        >>> p._insertkey('bar', 6)
        Traceback (most recent call last):
        KeyError: 'Key bar already exists as subdictionary'
        
        '''
        path = keypath.split(self.__separator)
        key = path.pop()
        dest = self
        for dir in path:
            if dir in dest:
                dest = dest[dir]
            else:
                dest[dir] = {}
                dest = dest[dir]
        if key in dest and isinstance(dest[key], dict):
            raise KeyError('Key %s already exists as subdictionary' % key)
        dest[key] = value

    def _readfile(self, infile, separator = "."):
        for line in infile:
            line2 = line.split('#', 1)[0].strip()
            if not line2:
               continue
            if not '=' in line2:
                raise Exception('Invalid line, missing "=": %s' % line)
            # Which one is worse?
            # (key, value) = re.match(r'(\S+)\s*=\s*(\S+)', line).groups()
            (key, value) = (s.strip() for s in line2.split('=', 1))
            self._insertkey(key, value)
            

if __name__ == "__main__":
    import doctest
    doctest.testmod()
