	type iarr =  array[1..256] of integer;
	type parr =  array[1..256] of real;
	type cct =  array[1..257] of char;
	type ivarray =  array[1..4,1..4] of real;
	type ivarray1 =  array[1..3,1..3] of real;
	type  pttype = record
		   x,y,z,w:real;
		end;

	type aspect = record
		   width, height:real; 
		end;

	type  primattr = record
		   lineindx: integer;
		   fillindx: integer;
		   textindx: integer;
		   linestyl: integer;
		   polyintstyl: integer;
		   polyedgstyl: integer;
		   linwidth: real;
		   pen: integer;
		   font: integer;
		   charsize: aspect;
		   chrup, chrpath, chrspace: pttype;
		   chjust: integer;
		   chqualty: integer;
		   marker: integer;
		   pickid: integer;
		   rasterop: integer; 
		end;

	type rasttyp =  record
				width: integer;
				height: integer;
				depth: integer;
				bits: integer; {var}
			end;
	type cmap =  	record
				typ: integer;
				nbyt: integer;
				dat :integer; {var}
			end;

	type  windtype = record 
					xmin, xmax, ymin, ymax:real;
			end;

	type  porttype = record 
					xmin,xmax,ymin,ymax,zmin,zmax:real;
			end;


	type vwprmtype = record
      				vwrefpt: array [1..3] of real;
      				vwplnorm: array [1..3] of real;
      				viewdis:real;
      				frontdis:real;
      				backdis:real;
      				projtype:integer;
      				projdir: array [1..3] of real;
      				window:windtype;
      				vwupdir: array [1..3] of real;
      				viewport:porttype;
      			 end;
	type vwsurf = record
				screenname: array [1..DEVNAMESIZE] of char;
				windowname: array [1..DEVNAMESIZE] of char;
				windowfd: integer;
				dd: integer;
				instance: integer;
				cmapsize: integer;
				cmapname: array [1..DEVNAMESIZE] of char;
				flags: integer;
				ptr: integer;
		     end;

	type vsurfst =  array[1..DEVNAMESIZE] of char;

	type vwarr = array[1..MAXVSURF] of vwsurf;
