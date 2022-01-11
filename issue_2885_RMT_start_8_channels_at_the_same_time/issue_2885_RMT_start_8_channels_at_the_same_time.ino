extern "C"
{
// includes local RMT Driver
#include "rmt_modified.h"
}

const size_t dataSize = 1500;
uint8_t* data;

// selected to be in order on the board for easy connection of the
// logic analyser
const uint8_t ChannelPins[] = {19, 18, 5, 17, 16, 4, 2, 15};

static void IRAM_ATTR _translate(const void* src,
                                 rmt_item32_t* dest,
                                 size_t src_size,
                                 size_t wanted_num,
                                 size_t* translated_size,
                                 size_t* item_num) {
  if (src == NULL || dest == NULL) {
    *translated_size = 0;
    *item_num = 0;
    return;
  }

  size_t size = 0;
  size_t num = 0;
  uint8_t *psrc = (uint8_t *)src;
  rmt_item32_t* pdest = dest;

  for (;;) {
    uint8_t data = *psrc;

    // convert a byte into rmt item timing
    // zero bit pulse = 200ns 1000ns = 8 cycles 40 cycles  = 0x0028 8008 as rmt item val
    // one bit pulse = 1000ns 200ns = 40 cycles 8 cycles  = 0x0008 8028 as rmt item val
    for (uint8_t bit = 0; bit < 8; bit++) {
      pdest->val = (data & 0x80) ? 0x00088028 : 0x00288008;
      pdest++;
      data <<= 1;
    }
    num += 8;
    size++;

    // if this is the last byte we need to adjust the length of the last pulse
    if (size >= src_size) {
      // extend the last bits LOW value to include the full reset signal length
      pdest--;
      pdest->duration1 = 20000; // 500us reset
      // and stop updating data to send
      break;
    }

    if (num >= wanted_num) {
      // stop updating data to send
      break;
    }

    psrc++;
  }

  *translated_size = size;
  *item_num = num;
}

void initChannel(rmt_channel_t ch, gpio_num_t pin) {
  rmt_config_t config;

  config.rmt_mode = RMT_MODE_TX;
  config.channel = ch;
  config.gpio_num = pin;
  config.mem_block_num = 1;
  config.tx_config.loop_en = false;

  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

  config.tx_config.carrier_en = false;
  config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;

  config.clk_div = 2;

  rmt_config(&config);
  rmt_driver_install(ch, 0, 0);
  rmt_translator_init(ch, _translate);
}

void writeChannel(rmt_channel_t ch) {
  // wait for the last send to complete
  if (ESP_OK == rmt_wait_tx_done(ch, 10000 / portTICK_PERIOD_MS)) {
    // then start a new async send
    rmt_write_sample_no_start(ch, data, dataSize); // just write the data to the RMT channel memory - do not start yet!
    //rmt_write_sample(ch, data, dataSize, false); // replaced by modified new function rmt_write_sample_no_start()
  }
}

void waitForAllDone()
{
  bool done = false;

  while (!done) {
    done = true;
    for (uint8_t ch = 0; ch < RMT_CHANNEL_MAX; ch++) {
      if (ESP_OK != rmt_wait_tx_done(static_cast<rmt_channel_t>(ch), 0)) {
        done = false;
        yield();
        break;
      }
    }
  }
}


void setup() {
  data = (uint8_t*)malloc(dataSize);
  memset(data, 0x00, dataSize);

  for (uint8_t ch = 0; ch < RMT_CHANNEL_MAX; ch++) {
    initChannel(static_cast<rmt_channel_t>(ch), static_cast<gpio_num_t>(ChannelPins[ch]));
  }
}

void loop() {
  // start all channels, then wait for them to be sent, then start all channels again
  //
  for (uint8_t ch = 0; ch < RMT_CHANNEL_MAX; ch++) {
    writeChannel(static_cast<rmt_channel_t>(ch));  // writeChannel() calls rmt_write_sample_no_start() instead of rmt_write_sample()
  }

  // new function added to the "modified RMT driver":
  rmt_start_all_channels();   // after writing the information, try to start all channel as fast as possible

  waitForAllDone();

  // repeat with a long delay between so it can easily be captured in the logic analyser
  delay(5000);
}
