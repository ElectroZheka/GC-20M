#ifndef TOOCHSCREEN_INCLUDED
#define TOOCHSCREEN_INCLUDED

void getCoordinates();

//=============================================================================================================================
void getCoordinates()
{
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif
}

#endif