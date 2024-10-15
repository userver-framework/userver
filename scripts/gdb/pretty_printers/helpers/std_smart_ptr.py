def dereference(sptr):
    if not sptr or not sptr['_M_ptr']:
        return None
    return sptr['_M_ptr'].dereference()
