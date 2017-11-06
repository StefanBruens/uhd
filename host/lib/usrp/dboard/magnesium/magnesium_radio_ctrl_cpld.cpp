//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "magnesium_radio_ctrl_impl.hpp"
#include "magnesium_cpld_ctrl.hpp"
#include "magnesium_constants.hpp"
#include <uhd/utils/log.hpp>

/*
 * Magnesium Rev C frequency bands:
 *
 * RX IF frequency is 2.4418 GHz. Have 80 MHz of bandwidth for loband.
 * TX IF frequency is 1.8-2.1 GHz (1.95 GHz is best).
 *
 * For RX:
 *     Band      SW2-AB SW3-ABC SW4-ABC SW5-ABCD SW6-ABC SW7-AB SW8-AB MIX
 *     WB        RF1 01 OFF 111 NA  --- NA  ---- RF3 001 RF2 01 RF2 01 0
 *     LB        RF2 10 RF5 100 NA  --- RF3 0010 RF1 100 RF1 10 RF1 10 1
 *     440-530   RF2 10 RF2 001 NA  --- RF1 1000 RF1 100 RF2 01 RF2 01 0
 *     650-1000  RF2 10 RF6 101 NA  --- RF4 0001 RF1 100 RF2 01 RF2 01 0
 *     1100-1575 RF2 10 RF4 011 NA  --- RF2 0100 RF1 100 RF2 01 RF2 01 0
 *     1600-2250 RF2 10 RF3 010 RF2 010 NA  ---- RF2 010 RF2 01 RF2 01 0
 *     2100-2850 RF2 10 RF1 000 RF1 100 NA  ---- RF2 010 RF2 01 RF2 01 0
 *     2700+     RF3 11 OFF 111 RF3 001 NA  ---- RF2 010 RF2 01 RF2 01 0
 *
 * For TX:
 *     Band      SW5-AB SW4-AB SW3-X SW2-ABCD SW1-AB SWTRX-AB MIX
 *     WB        RF1 10 RF2 01 RF1 0 NA  ---- SHD 00 RF4   11 0
 *     LB        RF2 01 RF1 10 RF2 1 RF3 0010 RF3 11 RF1   00 1
 *     <800      RF1 10 RF2 01 RF2 1 RF3 0010 RF3 11 RF1   00 0
 *     800-1700  RF1 10 RF2 01 RF2 1 RF2 0100 RF2 10 RF1   00 0
 *     1700-3400 RF1 10 RF2 01 RF2 1 RF1 1000 RF1 01 RF1   00 0
 *     3400-6400 RF1 10 RF2 01 RF2 1 RF4 0001 SHD 00 RF2   10 0
 */

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

void magnesium_radio_ctrl_impl::_update_atr_switches(
    const magnesium_cpld_ctrl::chan_sel_t chan,
    const direction_t dir,
    const std::string &ant
){
    magnesium_cpld_ctrl::rx_sw1_t rx_sw1 = magnesium_cpld_ctrl::RX_SW1_RX2INPUT;
    magnesium_cpld_ctrl::sw_trx_t sw_trx = _sw_trx[chan];

    bool trx_led = false, rx2_led = true;
    //bool tx_pa_enb = true, tx_amp_enb = true, tx_myk_en=true;
    if (ant == "TX/RX" and dir == RX_DIRECTION) {
        rx_sw1 = magnesium_cpld_ctrl::RX_SW1_TRXSWITCHOUTPUT;
        sw_trx = magnesium_cpld_ctrl::SW_TRX_RXCHANNELPATH;
        trx_led = true;
        rx2_led = false;
    }
    UHD_LOG_TRACE(unique_id(), "Update all atr related switches for " << dir << " " << ant );
    if (dir == RX_DIRECTION){
        _cpld->set_rx_atr_bits(
            chan,
            magnesium_cpld_ctrl::ON,
            rx_sw1,
            trx_led,
            rx2_led,
            true,
            true,
            true,
            true
        );
        _cpld->set_tx_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE,
            false,
            sw_trx,
            false,
            false,
            false
        );
        _cpld->set_rx_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE,
            rx_sw1,
            false,
            false,
            false,
            false,
            false,
            true
        );
    }
    if (dir == TX_DIRECTION){
        _cpld->set_tx_atr_bits(
            chan,
            magnesium_cpld_ctrl::ON,
            true,
            sw_trx,
            true,
            true,
            true
        );
        _cpld->set_rx_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE,
            rx_sw1,
            false,
            false,
            false,
            false,
            false,
            true
        );
    };
}

void magnesium_radio_ctrl_impl::_update_rx_freq_switches(
    const double freq,
    const size_t chan
) {
     UHD_LOG_TRACE(unique_id(),
             "Update all RX freq related switches. f=" << freq << " Hz, "
             "chan=" << chan);

     // Set filters based on frequency. Note: We always switch both channels
    if (freq < MAGNESIUM_RX_BAND1_MIN_FREQ) {
        _cpld->set_rx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
            magnesium_cpld_ctrl::RX_SW3_FILTER0490LPMHZ,
            magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
            magnesium_cpld_ctrl::RX_SW5_FILTER0490LPMHZFROM,
            magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_LOBAND,
            true
        );
    } else if (freq < MAGNESIUM_RX_BAND2_MIN_FREQ) {
        _cpld->set_rx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
            magnesium_cpld_ctrl::RX_SW3_FILTER0440X0530MHZ,
            magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
            magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
            magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false
        );
    } else if (freq < MAGNESIUM_RX_BAND3_MIN_FREQ) {
        _cpld->set_rx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
            magnesium_cpld_ctrl::RX_SW3_FILTER0650X1000MHZ,
            magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
            magnesium_cpld_ctrl::RX_SW5_FILTER0650X1000MHZFROM,
            magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false
        );
    } else if (freq < MAGNESIUM_RX_BAND4_MIN_FREQ) {
        _cpld->set_rx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
            magnesium_cpld_ctrl::RX_SW3_FILTER1100X1575MHZ,
            magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
            magnesium_cpld_ctrl::RX_SW5_FILTER1100X1575MHZFROM,
            magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false
        );
    } else if (freq < MAGNESIUM_RX_BAND5_MIN_FREQ) {
        _cpld->set_rx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
            magnesium_cpld_ctrl::RX_SW3_FILTER1600X2250MHZ,
            magnesium_cpld_ctrl::RX_SW4_FILTER1600X2250MHZFROM,
            magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
            magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false
        );
    } else if (freq < MAGNESIUM_RX_BAND6_MIN_FREQ) {
        _cpld->set_rx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3,
            magnesium_cpld_ctrl::RX_SW3_FILTER2100X2850MHZ,
            magnesium_cpld_ctrl::RX_SW4_FILTER2100X2850MHZFROM,
            magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
            magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false
        );
    } else {
        _cpld->set_rx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::RX_SW2_UPPERFILTERBANKTOSWITCH4,
            magnesium_cpld_ctrl::RX_SW3_SHUTDOWNSW3,
            magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ,
            magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM,
            magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false
        );
    }
}

void magnesium_radio_ctrl_impl::_update_tx_freq_switches(
    const double freq,
    const size_t chan
){
     UHD_LOG_TRACE(unique_id(),
             "Update all TX freq related switches. f=" << freq << " Hz, "
             "chan=" << chan);
     magnesium_cpld_ctrl::chan_sel_t chan_sel =
         _master ?  magnesium_cpld_ctrl::CHAN1 : magnesium_cpld_ctrl::CHAN2;

     // Set filters based on frequency
    if (freq < MAGNESIUM_TX_BAND1_MIN_FREQ) {
        _cpld->set_tx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1,
            magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP0800MHZ,
            magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP0800MHZ,
            magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_LOBAND,
            true,
            magnesium_cpld_ctrl::ON
        );
        _sw_trx[chan_sel] = magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
    } else if (freq < MAGNESIUM_TX_BAND2_MIN_FREQ) {
        _cpld->set_tx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1,
            magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP0800MHZ,
            magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP0800MHZ,
            magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false,
            magnesium_cpld_ctrl::ON
        );
        _sw_trx[chan_sel] = magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
    } else if (freq < MAGNESIUM_TX_BAND3_MIN_FREQ) {
        _cpld->set_tx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1,
            magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP1700MHZ,
            magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP1700MHZ,
            magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false,
            magnesium_cpld_ctrl::ON
        );
        _sw_trx[chan_sel] = magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
    } else if (freq < MAGNESIUM_TX_BAND4_MIN_FREQ) {
        _cpld->set_tx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1,
            magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP3400MHZ,
            magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP3400MHZ,
            magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false,
            magnesium_cpld_ctrl::ON
        );
        _sw_trx[chan_sel] = magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
    } else {
        _cpld->set_tx_switches(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::SW_TRX_FROMTXUPPERFILTERBANKLP6400MHZ,
            magnesium_cpld_ctrl::TX_SW1_SHUTDOWNTXSW1,
            magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP6400MHZ,
            magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS,
            magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS,
            false,
            magnesium_cpld_ctrl::ON
        );
        _sw_trx[chan_sel] = magnesium_cpld_ctrl::SW_TRX_FROMTXUPPERFILTERBANKLP6400MHZ;
    }
}


