<?php setcookie("X-Purpose", $_SERVER["HTTP_X_PURPOSE"]);

if (isset($_COOKIE['X-Purpose'])) {
    setcookie("X-Purpose", "", time() - 3600);
    echo "<h1>The cookie was set!</h1>";
    echo "<p>X-Purpose: ";
    echo $_COOKIE['X-Purpose'];
} else {
    echo "<h1>BAD BROWSER NO COOKIE</h1>";
}
?>

<script>
layoutTestController.notifyDone();
</script>

<p>This test verifies that prefetches are sent with the HTTP request header
<b>X-Purpose: prefetch</b>.  To do this, the root page has a prefetch
link targetting this subresource which contains a PHP script
(resources/prefetch-purpose.php).  That initial prefetch of this
resource sets a cookie.  Later, the root page sets window.location to
target this script, which verifies the presence of the cookie, and
generates the happy test output that you hopefully see right now.

