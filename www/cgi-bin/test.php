<?php
header("Content-Type: text/html; charset=UTF-8");

// Vérification de la méthode de la requête
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $name = htmlspecialchars($_POST['name'] ?? 'Guest');
    echo "<h1>Welcome, $name!</h1>";
} else {
    echo "<h1>Hello! This is a static message from PHP.</h1>";
}
?>

