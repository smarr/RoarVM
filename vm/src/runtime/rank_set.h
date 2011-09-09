/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    David Ungar, IBM Research - Initial Implementation
 *    Sam Adams, IBM Research - Initial Implementation
 *    Stefan Marr, Vrije Universiteit Brussel - Port to x86 Multi-Core Systems
 ******************************************************************************/


class Rank_Set {
  typedef u_int64 contents_t;

  contents_t _contents;


public:
  static void unit_test();

  static int capacity() { return sizeof(contents_t) * 8; }

  Rank_Set() { _contents = 0LL; }
  Rank_Set(const Rank_Set& rs) { _contents = rs._contents; }

  static Rank_Set all_up_to(const int x) {
    Rank_Set r;
    r._contents = contents_for_all_up_to(x);
    return r;
  }
  static Rank_Set with_contents(const contents_t x) {
    Rank_Set r;
    r._contents = x;
    return r;
  }

  contents_t contents() const { return _contents; }

  bool is_empty() { return contents() == 0LL; }
  static const int none = -1;
  int  first_or_none() { return is_empty() ? none : least_significant_bit_position(contents()); }

  void add(int x) { _contents |= bit_for(x); }
  void remove(int x) { _contents &= ~bit_for(x); }
  bool includes(int x)  const { return (_contents & bit_for(x)) ? true : false; }

  Rank_Set operator + (const Rank_Set rs) const {
    return Rank_Set::with_contents(contents() |  rs.contents());
  }

  Rank_Set operator + (int x) const {
    return Rank_Set::with_contents(contents() |  bit_for(x));
  }

  Rank_Set operator - (const Rank_Set rs) const {
    return Rank_Set::with_contents(contents() & ~rs.contents());
  }

  Rank_Set operator - (int x) const {
    return Rank_Set::with_contents(contents() & ~bit_for(x));
  }

  bool operator  == (const Rank_Set rs) const { return contents() == rs.contents(); }
  bool operator  != (const Rank_Set rs) const { return contents() != rs.contents(); }

 private:
  static contents_t bit_for(int x) { return 1LL << contents_t(x); }
  static contents_t contents_for_all_up_to(int x)  { return x == 64 ? ~0ULL : bit_for(x) - 1ULL; }

  void verify_includes_only(int a = -1, int b = -1) const;

};


# define FOR_EACH_RANK_IN_SET(set, rank) \
  for (int rank = 0;  rank < Rank_Set::capacity();  ++rank) if (!set.includes(rank)); else

