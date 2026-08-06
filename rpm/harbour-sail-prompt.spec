Name:       harbour-sail-prompt

Summary:    Conversational AI assistant with Markdown export
Version:    0.1
Release:    1
License:    BSD-3-Clause
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   sailfish-components-webview-qt5
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  desktop-file-utils
BuildRequires:  cmake

%description
Sail Prompt is an AI chat application for Sailfish OS. It requires access to an
OpenAI-compatible chat completion API.


%prep
%setup -q -n %{name}-%{version}

%build

%cmake

%make_build


%install
%make_install


desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
