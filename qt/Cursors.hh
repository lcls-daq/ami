#ifndef AmiQt_Cursors_hh
#define AmiQt_Cursors_hh

namespace Ami {
  namespace Qt {
    class PlotFrame;
    class Cursors {
    public:
      Cursors(PlotFrame&);
      virtual ~Cursors();
    public:
      void set_cursor (double, double);
      void grab_cursor();
    private:
      virtual void _set_cursor(double, double) = 0;
    protected:
      PlotFrame&  _frame;
    };
  };
};

#endif
