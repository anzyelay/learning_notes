```vala
public class AsyncMutex {
    private class Callback {
        public SourceFunc callback;

        public Callback (owned SourceFunc cb) {
            callback = (owned)cb;
        }
    }

    private Gee.ArrayQueue<Callback> callbacks;
    private bool locked;

    public AsyncMutex () {
        locked = false;
        callbacks = new Gee.ArrayQueue<Callback> ();
    }

    public async void lock () {
        while (locked) {
            SourceFunc cb = lock.callback;
            callbacks.offer_head (new Callback ((owned)cb));
            yield;
        }

        locked = true;
    }

    public void unlock () {
        locked = false;
        var callback = callbacks.poll_head ();
        if (callback != null) {
            Idle.add ((owned)callback.callback);
        }
    }
}

```
