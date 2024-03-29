Name:		libltnsdi
Version:	1.0.0
Release:	1
Summary:	SDI related tools

License:	GPLv2+
URL:		www.ltnglobal.com

#BuildRequires:	
BuildRequires:	zlib-devel
BuildRequires:	libpcap-devel
BuildRequires:	ncurses-devel

Requires:	zlib
Requires:	libpcap
Requires:	ncurses

%description
A tool to inspect certain SDI stream attributes from BM cards.

%files
/usr/local/bin/ltnsdi_util
/usr/local/bin/ltnsdi_demo
/usr/local/bin/ltnsdi_audio_analyzer

%changelog
* Fri Dec  9 2022 Steven Toth <stoth@ltnglobal.com> 
- v1.1.0
  ltnsdi_audio_analyzer: Replace Grp/Ch labels with Pair/Ch labels
  ltnsdi_audio_analyzer: Fix column header issue for dbFS in console report
  ltnsdi_audio_analyzer: Detect 8 or 16 channels from DUO vs DUO2 and adjust display accordingly.

* Thu May 12 2022 Steven Toth <stoth@ltnglobal.com> 
- v1.0.0
  ltnsdi_audio_analyzer: Release to production.
  ltnsdi_audio_analyzer: dbFS precision fixes.

