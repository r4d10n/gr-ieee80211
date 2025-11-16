/* -*- c++ -*- */
/*
 * Copyright 2017 Teng-Hui Huang.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifndef INCLUDED_IEEE80211_DSSS_CHIP_SYNC_C_H
#define INCLUDED_IEEE80211_DSSS_CHIP_SYNC_C_H

#include <gnuradio/ieee80211/api.h>
#include <gnuradio/block.h>
#include <memory>

namespace gr {
  namespace ieee80211 {

    /*!
     * \brief DSSS/CCK chip synchronization and demodulation block
     * \ingroup ieee80211
     *
     * This block performs chip-level synchronization for 802.11b DSSS/CCK signals.
     * It supports 1, 2, 5.5, and 11 Mbps rates with both long and short preambles.
     */
    class IEEE80211_API chip_sync_c : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<chip_sync_c> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of ieee80211::chip_sync_c.
       *
       * To avoid accidental use of raw pointers, ieee80211::chip_sync_c's
       * constructor is in a private implementation
       * class. ieee80211::chip_sync_c::make is the public interface for
       * creating new instances.
       */
      static sptr make(bool longPre, float threshold);

      virtual void set_preamble_type(bool islong)=0;
    };

  } // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_DSSS_CHIP_SYNC_C_H */
