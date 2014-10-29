#include "ami/qt/CursorOp.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/RawFilter.hh"

#include <stdio.h>

using namespace Ami::Qt;

void CursorOp::configure(char*& p, unsigned input, unsigned& output,
			 ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			 const AxisInfo& xinfo, ConfigureRequest::Source source)
{
  std::map<std::string,double> values;
  configure(p, input, output, channels, signatures, nchannels, xinfo, source, values);
}

void CursorOp::configure(char*& p, unsigned input, unsigned& output,
			 ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			 const AxisInfo& xinfo, ConfigureRequest::Source source,
			 const std::map<std::string,double>& values)
{
  unsigned channel = _channel;
  unsigned input_signature = signatures[channel];
  configure(p, input_signature, output, xinfo, source, values);
}

void CursorOp::configure(char*& p, unsigned input, unsigned& output,
			 const AxisInfo& xinfo, ConfigureRequest::Source source)
{
  std::map<std::string,double> values;
  configure(p, input, output, xinfo, source, values);
}

void CursorOp::configure(char*& p, unsigned input, unsigned& output,
			 const AxisInfo& xinfo, ConfigureRequest::Source source,
			 const std::map<std::string,double>& values)
{
  QString expr(_input->expression());

  // replace cursor names with values
  if (values.size()) {
    for(std::map<std::string,double>::const_iterator it=values.begin(); it!=values.end(); it++) {
      QString new_expr;
      const QString match(it->first.c_str());
      int last=0;
      int pos=0;
      while( (pos=expr.indexOf(match,pos)) != -1) {
	new_expr.append(expr.mid(last,pos-last));
	new_expr.append(QString("[%1]").arg(it->second));
	pos += match.size();
	last = pos;
      }
      new_expr.append(expr.mid(last));
      expr = new_expr;
    }
  }

  // replace cursor values with bin indices
  QString new_expr;
  { QRegExp match("\\[[^\\]]*\\]");
    int last=0;
    int pos=0;
    while( (pos=match.indexIn(expr,pos)) != -1) {
      QString use = expr.mid(pos+1,match.matchedLength()-2);
      bool ok;
      double v = use.toDouble(&ok);
      unsigned bin=0;
      if (!ok)
	printf("error parsing double %s\n",qPrintable(use));
      else {
	bin = xinfo.tick(v);
      }
      new_expr.append(expr.mid(last,pos-last));
      new_expr.append(QString("[%1]").arg(bin));
      pos += match.matchedLength();
      last = pos;
    }
    new_expr.append(expr.mid(last));
    new_expr.replace(QString("]%1[").arg(BinMath::integrate()),QString(BinMath::integrate()));
    new_expr.replace(QString("]%1[").arg(BinMath::moment1  ()),QString(BinMath::moment1  ()));
    new_expr.replace(QString("]%1[").arg(BinMath::moment2  ()),QString(BinMath::moment2  ()));
    new_expr.replace(QString("]%1[").arg(BinMath::range    ()),QString(BinMath::range    ()));
    new_expr.replace(QString("]%1[").arg(BinMath::contrast ()),QString(BinMath::contrast ()));
    new_expr.replace(QString("]%1[").arg(BinMath::xmoment  ()),QString(BinMath::xmoment  ()));
    new_expr.replace(QString("]%1[").arg(BinMath::ymoment  ()),QString(BinMath::ymoment  ()));
    new_expr.replace(QString("]%1[").arg(BinMath::mean     ()),QString(BinMath::mean     ()));
    new_expr.replace(QString("]%1[").arg(BinMath::variance ()),QString(BinMath::variance ()));
  }

  QString end_expr;
  { int last=0, next=0, pos=0;
    while( (pos=new_expr.indexOf(BinMath::range(),pos)) != -1) {
      if ( (next=new_expr.lastIndexOf("[",pos))==-1 )
	printf("error parsing range in %s\n",qPrintable(expr));
      else {
	end_expr.append(new_expr.mid(last,next-last));
	last  = new_expr.indexOf("]",pos);
	int a = new_expr.mid(next+1,pos -next-1).toInt();
	int b = new_expr.mid(pos +1,last-pos -1).toInt();
	printf("%s/%d %s/%d\n",
	       qPrintable(new_expr.mid(next+1,pos -next-1)),a,
	       qPrintable(new_expr.mid(pos +1,last-pos -1)),b);
	end_expr.append(QString("(%1)").arg(QString::number(abs(a-b)+1)));
	pos  = ++last;
      }
    }
    end_expr.append(new_expr.mid(last));
  }

  Ami::BinMath op(_input->output(), qPrintable(end_expr));
  
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  source,
						  input,
						  -1,
                                                  RawFilter(),
						  op);
  p += r.size();
  _req.request(r,output);
  _output_signature = r.output();
}
