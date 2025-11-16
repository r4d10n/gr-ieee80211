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


#ifndef INCLUDED_IEEE80211_DSSS_PPDU_CHIP_MAPPER_BC_H
#define INCLUDED_IEEE80211_DSSS_PPDU_CHIP_MAPPER_BC_H

#include <gnuradio/ieee80211/api.h>
#include <gnuradio/block.h>
#include <memory>

namespace gr {
  namespace ieee80211 {

    /*!
     * \brief DSSS/CCK chip mapper block
     * \ingroup ieee80211
     *
     * Maps PPDU bytes to DSSS/CCK chips for 802.11b transmission.
     * Supports all 802.11b rates (1, 2, 5.5, 11 Mbps) with long and short preambles.
     */
    class IEEE80211_API ppdu_chip_mapper_bc : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<ppdu_chip_mapper_bc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of ieee80211::ppdu_chip_mapper_bc.
       *
       * To avoid accidental use of raw pointers, ieee80211::ppdu_chip_mapper_bc's
       * constructor is in a private implementation
       * class. ieee80211::ppdu_chip_mapper_bc::make is the public interface for
       * creating new instances.
       */
      static sptr make(const std::string& lentag);
    };

  } // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_DSSS_PPDU_CHIP_MAPPER_BC_H */
