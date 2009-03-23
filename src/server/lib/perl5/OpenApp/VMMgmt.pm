package OpenApp::VMMgmt;

use strict;

our $OPENAPP_ID = 'openapp';
our $OPENAPP_DNAME = 'OpenAppliance';
our $OPENAPP_SNMP_COMM = 'openappliance';
our $OPENAPP_VENDOR = 'Vyatta';
our $OPENAPP_BFORMAT = '1';

my $VMDIR = '/opt/vyatta/etc/gui/VM';
my $STATUS_DIR = '/opt/vyatta/var/run/vmstatus';
my $HWMON_FILE = '/opt/vyatta/var/run/vm-monitor.hw';

### "static" functions
sub getVMList {
  my $dd = undef;
  opendir($dd, "$VMDIR") or return;
  my @v = grep { !/^\./ && -f "$VMDIR/$_" } readdir($dd);
  closedir($dd);
  return @v;
}

sub updateStatus {
  my ($id, $st, $cpu, $dall, $dfree, $mall, $mfree, $upd) = @_;
  my $fd = undef;
  open($fd, '>', "$STATUS_DIR/$id") or return;
  print $fd "$st $cpu $dall $dfree $mall $mfree $upd\n";
  close($fd);
}

sub updateHwMon {
  my ($nic, $disk, $cpu, $fan) = @_;
  my $fd = undef;
  open($fd, '>', "$HWMON_FILE") or return;
  print $fd <<EOF;
nic $nic
disk $disk
cpu $cpu
fan $fan
EOF
  close($fd);
}

sub getHwMonData {
  my ($nic, $disk, $cpu, $fan) = ('unknown', 'unknown', 'unknown', 'unknown');
  my $fd = undef;
  open($fd, '<', "$HWMON_FILE") or return;
  while (<$fd>) {
    chomp;
    my @words = split(/ /);
    if ($words[0] eq 'nic') {
      $nic = $words[1];
    } elsif ($words[0] eq 'disk') {
      $disk = $words[1];
    } elsif ($words[0] eq 'cpu') {
      $cpu = $words[1];
    } elsif ($words[0] eq 'fan') {
      $fan = $words[1];
    }
  }
  close($fd);
  return ($nic, $disk, $cpu, $fan);
}

### data
my %fields = (
  # metadata
  _vmId => undef,
  _vmIP => undef,
  _vmWuiPort => undef,
  _vmWuiUri => undef,
  _vmImgVer => undef,
  _vmVendor => undef,
  _vmBackupFormat => undef,
  _vmDisplayName => undef,

  # status
  _vmState => undef,
  _vmCpu => undef,
  _vmDiskAll => undef,
  _vmDiskFree => undef,
  _vmMemAll => undef,
  _vmMemFree => undef,
  _vmNewUpdate => undef
);

sub _isValidId {
  my ($id) = @_;
  return ($id eq $OPENAPP_ID || -r "$VMDIR/$id") ? 1 : 0;
}

sub _setupMeta {
  my ($self, $id) = @_;
  if ($id eq $OPENAPP_ID) {
    # dom0
    $self->{_vmId} = $OPENAPP_ID;
    $self->{_vmVendor} = $OPENAPP_VENDOR;
    $self->{_vmBackupFormat} = $OPENAPP_BFORMAT;
    $self->{_vmDisplayName} = $OPENAPP_DNAME;
    return;
  }
  my $fd = undef;
  open($fd, '<', "$VMDIR/$id") or return;
  my $data = <$fd>;
  close($fd);
  chomp($data);
  my ($ip, $port, $uri, $ver, $vend, $bform, $dname) = split(/ /, $data, 7);
  $self->{_vmId} = $id;
  $self->{_vmIP} = $ip;
  $self->{_vmWuiPort} = $port;
  $self->{_vmWuiUri} = $uri;
  $self->{_vmImgVer} = $ver;
  $self->{_vmVendor} = $vend;
  $self->{_vmBackupFormat} = $bform;
  $self->{_vmDisplayName} = $dname;
}

sub refreshStatus {
  my ($self) = @_;
  my $id = $self->{_vmId};
  if (! -r "$STATUS_DIR/$id") {
    return;
  }
  my $fd = undef;
  open($fd, '<', "$STATUS_DIR/$id") or return;
  my $data = <$fd>;
  close($fd);
  chomp($data);
  my ($st, $cpu, $dall, $dfree, $mall, $mfree, $upd) = split(/ /, $data);
  $self->{_vmState} = $st;
  $self->{_vmCpu} = $cpu;
  $self->{_vmDiskAll} = $dall;
  $self->{_vmDiskFree} = $dfree;
  $self->{_vmMemAll} = $mall;
  $self->{_vmMemFree} = $mfree;
  $self->{_vmNewUpdate} = $upd;
}

sub _setup {
  my ($self, $id) = @_;
  $self->_setupMeta($id);
  $self->refreshStatus();
}

sub new {
  my ($that, $id) = @_;
  if (!isValidId($id)) {
    return undef;
  }
  my $class = ref ($that) || $that;
  my $self = {
    %fields,
  };

  bless $self, $class;
  $self->_setup($id);
  return $self;
}

### getters for VM metadata
sub getId {
  my ($self) = @_;
  return $self->{_vmId};
}

sub getIP {
  my ($self) = @_;
  return $self->{_vmIP};
}

sub getWuiPort {
  my ($self) = @_;
  return $self->{_vmWuiPort};
}

sub getWuiUri {
  my ($self) = @_;
  return $self->{_vmWuiUri};
}

sub getImgVer {
  my ($self) = @_;
  return $self->{_vmImgVer};
}

sub getDisplayName {
  my ($self) = @_;
  return $self->{_vmDisplayName};
}

### getters for VM status
sub getState {
  my ($self) = @_;
  return $self->{_vmState};
}

sub getCpu {
  my ($self) = @_;
  return $self->{_vmCpu};
}

sub getDiskAll {
  my ($self) = @_;
  return $self->{_vmDiskAll};
}

sub getDiskFree {
  my ($self) = @_;
  return $self->{_vmDiskFree};
}

sub getMemAll {
  my ($self) = @_;
  return $self->{_vmMemAll};
}

sub getMemFree {
  my ($self) = @_;
  return $self->{_vmMemFree};
}

sub getUpdateAvail {
  my ($self) = @_;
  return $self->{_vmNewUpdate};
}

1;

