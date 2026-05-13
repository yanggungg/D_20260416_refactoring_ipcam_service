using GLib;

public class NfSession : Object {
  private string dumpstr;
  private string _uniqid;
  private string _usrid;
  private uint _remote_port;
  private long _expire;
  private long _maxage;

  public string uniqid {
    get { return _uniqid; }
    set { _uniqid = value; }
  }

  public string usrid {
    get { return _usrid; }
    set { _usrid = value; }
  }

  public uint remote_port {
    get { return _remote_port; }
    set { _remote_port = value; }
  }

  public long expire {
    get { return _expire; }
    set { _expire = value; }
  }

  public long maxage {
    get { return _maxage; }
    set { _maxage = value; }
  }

  public NfSession(string _usrid = "", uint _remote_port = 0, long _maxage = 0)
    requires ( _usrid != null)
    requires ( _remote_port <= 65536 )
  {
    TimeVal tv = TimeVal();

    uniqid = _getuniqid(_remote_port);
    remote_port = _remote_port;

    if ( _maxage == 0 ) {
      _maxage = 3600; // default 1 hour
    }

    expire = tv.tv_sec + _maxage;
    maxage = _maxage;
    usrid = _usrid;

#if 0
    stdout.printf("%s usr[%s] port[%u] tv[%ld %ld] maxage[+%ld]\n", "NfSession Constructor", usrid, remote_port, tv.tv_sec, tv.tv_usec, _expire);
#endif
  }

  private string _getuniqid(uint remote_port = 0)
    requires ( remote_port <= 65536 )
  {
    TimeVal tv = TimeVal();

    #if TEST
    stdout.printf("%ld %ld %d %u\n", (uint)tv.tv_usec, (uint)tv.tv_sec, Posix.getpid(), remote_port%0x10000);
    #endif

    string uniqid = "%05x%05x%04x%04x".printf((uint)tv.tv_usec, (uint)tv.tv_sec, Posix.getpid(), remote_port%0x10000);

    return uniqid;
  }

  public bool isExpired() {
    TimeVal tv = TimeVal();

    if( this.expire < tv.tv_sec ) {
      // expired
      return true;
    }
    return false;
  }

  public void refreshExpire() {
    TimeVal tv = TimeVal();

    this.expire = tv.tv_sec + this.maxage;
  }

  public unowned string dump() {
    //dumpstr.printf( "[NfSession Dump] uniqid=%s expire=%ld\n", uniqid, expire);
    dumpstr = "[NfSession Dump] usrid=%s port=%u uniqid=%s expire=%ld\n".printf(
        usrid,
        remote_port,
        uniqid,
        expire);

    return dumpstr;
  }
}

const int SESSION_MAX = 20;
const string SESSION_FILE_PATH = "/tmp/session";

public class NfSessionManager : Object{
  Mutex mutex = new Mutex();

  /* Preference */
  private bool use_session_file = false;

  /* Members */
  private static NfSessionManager instance = null;

  private NfSessionManager( ) {
    sessionList = new List<NfSession>();
  }

  private string dumpstr;

  public static NfSessionManager getInstance() {
    if( instance == null ) {
      instance = new NfSessionManager();
    }
    return instance;
  }

  private List<NfSession> sessionList = null;

  public uint addSession(NfSession nfsession)
    requires ( nfsession != null )
  {
    unowned NfSession sess = findSession(nfsession.uniqid);

    mutex.lock();
    if( sess != null ) {
      stderr.printf("Session %s is exists, not appended\n", sess.uniqid);
      nfsession.unref();
      mutex.unlock();
      return sessionList.length();
    }

    if (sessionList.length() >= SESSION_MAX) {
      unowned NfSession sess0 = sessionList.nth_data(0);
      sessionList.remove(sess0);
      sess0.unref();
    }

    sessionList.append(nfsession);
    mutex.unlock();

    #if TEST
      stdout.printf("Adding nfsession uniqid=%s expire=%ld\n", nfsession.uniqid, nfsession.expire);
    #endif

    if( use_session_file ) {
      // if use session file is set, save session info into SESSION_FILE_PATH
    }
    return sessionList.length();
  }

  public void removeSession(NfSession? session)
    requires ( session != null )
  {
    mutex.lock();
    sessionList.remove(session);
    session.unref();
    mutex.unlock();
  }

  public unowned NfSession? findSession(string sessionId) {
    CompareFunc <NfSession> sessionCmpById = (a, b) => {
      return strcmp(a.uniqid, b.uniqid);
    };

    NfSession findSessId = new NfSession();
    findSessId.uniqid = sessionId;

    mutex.lock();
    unowned List<NfSession>? session_elem = sessionList.find_custom(findSessId, sessionCmpById);

    if( session_elem == null ) {
      mutex.unlock();
      return null;
    }

    if( session_elem.data.isExpired() ) {
      stdout.printf("session: %s is expired\n", session_elem.data.dump());
      mutex.unlock();
      removeSession(session_elem.data);

      return null;
    }
    mutex.unlock();

    return session_elem.data;
  }

  public unowned NfSession? findSessionByIdx(int index)
    requires ( index < sessionList.length() )
  {
    return sessionList.nth_data(index);
  }

  public unowned string dump() {
    int i = 0;
    mutex.lock();

    TimeVal tv = TimeVal();
    dumpstr =  "========= [User Session ========\n";
    dumpstr += "List Len=%u Now=%ld\n".printf(sessionList.length(), tv.tv_sec);
    foreach( NfSession sess in sessionList ) {
      //dumpstr.printf( "[%d] uniqid=%s expire=%ld\n", i++, sess.uniqid, sess.expire);
      dumpstr += "[%d:%p:%p] usrid=%s, port=%u, uniqid=%s expire=%ld[+%ld]\n".printf(
        i++, sess, sessionList,
        sess.usrid,
        sess.remote_port,
        sess.uniqid,
        sess.expire,
        sess.maxage);
    }
    dumpstr +=  "========= User Session] ========\n";

    mutex.unlock();

    return dumpstr;
  }

#if TEST
  public static int main(string[] args) {
    NfSessionManager manager = NfSessionManager.getInstance();
    NfSession sess;

    stdout.printf("===================================\n");
    manager.addSession(new NfSession("ADMIN1", 0 , 1000));
    manager.addSession(new NfSession("admin2", 10000, 2000));
    manager.addSession(new NfSession("guest3", 20000, 3000));
    manager.addSession(new NfSession("ADMIN4", 0 , 1000));
    manager.addSession(new NfSession("admin5", 10000, 2000));
    manager.addSession(new NfSession("guest6", 20000, 3000));
    manager.addSession(new NfSession("ADMIN7", 0 , 1000));
    manager.addSession(new NfSession("admin8", 10000, 2000));
    manager.addSession(new NfSession("guest9", 20000, 3000));
    manager.addSession(new NfSession("ADMIN0", 0 , 1000));
    manager.addSession(new NfSession("admin1", 10000, 2000));
    manager.addSession(new NfSession("guest2", 20000, 3000));
    manager.addSession(new NfSession("ADMIN3", 0 , 1000));
    manager.addSession(new NfSession("admin4", 10000, 2000));
    manager.addSession(new NfSession("guest5", 20000, 3000));
    manager.addSession(new NfSession("ADMIN6", 0 , 1000));
    manager.addSession(new NfSession("admin7", 10000, 2000));
    manager.addSession(new NfSession("guest8", 20000, 3000));

    for ( int i=0 ; i < 20 ; i++ ) {
      stdout.printf("===================================\n");
      stdout.puts(manager.dump());

      stdout.printf("==== finding by uniqid\n");
      sess = manager.findSession("49aa953a2480142610000");

      if( sess != null ) {
        stdout.puts(sess.dump());
      }

      GLib.Thread.usleep(100000);
    }

      stdout.printf("==== deleting null\n");
      manager.removeSession(null);

      stdout.puts(manager.dump());
      stdout.printf("===================================\n");

    for ( int i=0 ; i < 1000 ; i++ ) {
      stdout.printf("======== add Session===============\n");
      manager.addSession(new NfSession("ADMIN1", 0 , 1000));
      stdout.puts(manager.dump());
      stdout.printf("===================================\n");

      stdout.printf("==== finding by index 0\n");
      sess = manager.findSessionByIdx(0);

      if( sess != null ) {
	stdout.puts(sess.dump());
      }
      GLib.Thread.usleep(2000000);
    }


      stdout.printf("==== finding by index 0\n");
      sess = manager.findSessionByIdx(0);

      if( sess != null ) {
	stdout.puts(sess.dump());
      }
      stdout.printf("==== deleting index 0\n");
      manager.removeSession(sess);

      stdout.puts(manager.dump());
      stdout.printf("===================================\n");

      stdout.printf("==== finding by index 5\n");
      sess = manager.findSessionByIdx(5);

      if( sess != null ) {
	stdout.puts(sess.dump());
      }

      stdout.printf("==== deleting index 5\n");
      manager.removeSession(sess);

      stdout.puts(manager.dump());
      stdout.printf("===================================\n");


    return 0;
  }
#endif
}

