/**
 * Candlestick template library
 * v. 1.0
 * Copyright (C) 2016 Robert Ulbricht
 * http://www.arduinoslovakia.eu
 *
 * 3 different renderers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 

///
/// One OHLC bar data
///
template<typename ohlcvalue>
struct OHLCData {
  ohlcvalue o;
  ohlcvalue h;
  ohlcvalue l;
  ohlcvalue c;  
};

///
/// Serial port renderer
///
template<typename ohlcvalue>
class OHLCSerialRender {
public:
  void drawHeader() {
    Serial.println("###");
    }
  void drawGrid(ohlcvalue minimum, ohlcvalue maximum, ohlcvalue grid_step) {
    }
  void drawBar(int pos, OHLCData<ohlcvalue> *value) {
    Serial.print(pos);
    Serial.print(",");
    Serial.print(value->o);
    Serial.print(",");
    Serial.print(value->h);
    Serial.print(",");
    Serial.print(value->l);
    Serial.print(",");
    Serial.print(value->c);
    Serial.println("");
    }
  void drawFooter() {}
};

///
/// Nokia 5110 base renderer
///
template<class ohlcvalue>
class OHLCNokia5110BaseRender {
protected:
  Adafruit_PCD8544 *display;
public:
  OHLCNokia5110BaseRender() : display(NULL) {}

  void drawHeader() {}
  void drawFooter() {}
  void setDisplay(Adafruit_PCD8544 *d) {display=d;}
  void drawGrid(ohlcvalue minimum, ohlcvalue maximum, ohlcvalue grid_step) {
    const int HEIGHT = 47;
    int dv = minimum / grid_step;
    ohlcvalue bg = (dv + 0) * grid_step;
    for(ohlcvalue i = bg;i<maximum+grid_step;i+=grid_step) {
      for(int j=0;j<=84;j+=5)
        display->drawPixel(j,HEIGHT-(int)i,BLACK);
      }
    }
};

///
/// Nokia 5110 Line Candlestick renderer
///
template<typename ohlcvalue>
class OHLCNokia5110LineRender : public OHLCNokia5110BaseRender<ohlcvalue> {
public:
  void drawBar(int pos, OHLCData<ohlcvalue> *bar) {
    if(this->display==NULL)
      return;
      
    int START = 5 + pos * 6;
    const int BW = 2;
    const int HEIGHT = 47;

    // open
    this->display->drawLine(START,HEIGHT-bar->o,START+1*BW,HEIGHT-bar->o,BLACK);

    // high - low
    this->display->drawLine(START+1*BW,HEIGHT-bar->h,START+1*BW,HEIGHT-bar->l,BLACK);

    // close
    this->display->drawLine(START+1*BW,HEIGHT-bar->c,START+2*BW,HEIGHT-bar->c,BLACK);
    }
};

///
/// Nokia 5110 Bar Candlestick renderer
///
template<class ohlcvalue>
class OHLCNokia5110BarRender : public OHLCNokia5110BaseRender<ohlcvalue> {
public:
  void drawBar(int pos, OHLCData<ohlcvalue> *bar) {
    if(this->display==NULL)
      return;
      
    int START = 5 + pos * 6;
    const int BW = 2;
    const int HEIGHT = 47;
    int top;
    int bottom;

    if(bar->o < bar->c) {
      top = bar->c;
      bottom = bar->o;
    } else {
      top = bar->o;
      bottom = bar->c;
    }

    // high
    this->display->drawLine(START+1*BW,HEIGHT-bar->h,START+1*BW,HEIGHT-top,BLACK);

    // low
    this->display->drawLine(START+1*BW,HEIGHT-bar->l,START+1*BW,HEIGHT-bottom,BLACK);

    // bar
    if(bar->o < bar->c) {
      this->display->fillRect(START,HEIGHT-bottom,2*BW+1,bottom-top+1,WHITE);
      this->display->drawRect(START,HEIGHT-bottom,2*BW+1,bottom-top+1,BLACK);
      }
    else
      this->display->fillRect(START,HEIGHT-bottom,2*BW+1,bottom-top+1,BLACK);
    }
};

///
/// OHLC recorder and renderer
/// ohlcvalue - int or float
/// len - number of bars
/// stp - time window (10000 = 10 seconds)
/// renderclass - which renderer we want to use
///
template<typename ohlcvalue, const int len, const int stp, typename renderclass>
class OHLCChart {
  OHLCData<ohlcvalue> data[len];
  const int ohlc_step;
  int ohlc_first;
  OHLCData<ohlcvalue> *last;
  unsigned long ohlc_next_step;
  renderclass render;
  ohlcvalue grid_step;
  
public:
  OHLCChart() : ohlc_step(stp), ohlc_first(len), ohlc_next_step(0) {
    last = data + (len - 1);
  }

  renderclass& getRender() {
    return render;
  }

  void setGridStep(ohlcvalue value) {
    grid_step = value;
  }
  
  boolean addValue(ohlcvalue v) {
    unsigned long m=millis();
    if(m>ohlc_next_step) {
      if(ohlc_first>0)
        ohlc_first--;
      for(int i=0;i<(len-1);i++)
        data[i] = data[i+1];
      last->o = v;
      last->h = v;
      last->l = v;
      last->c = v;
      ohlc_next_step+=ohlc_step;
      return true;
    } else {
      if(last->h<v)
        last->h=v;
      if(last->l>v)
        last->l=v;
      last->c=v;
    }
  return false;
  }

  void draw() {
    render.drawHeader();
    if(grid_step>0 && ohlc_first!=len) {
      ohlcvalue minimum = data[ohlc_first].l;
      ohlcvalue maximum = data[ohlc_first].h;
      for(int i=ohlc_first+1;i<len;i++) {
        if(data[i].l<minimum)
          minimum = data[i].l;
        if(data[i].h>maximum)
          maximum = data[i].h;
      }
      if(minimum!=maximum)
        render.drawGrid(minimum,maximum,grid_step);
    }
    for(int i=ohlc_first;i<len;i++)
      render.drawBar(i,data+i);
    render.drawFooter();
  }
};

