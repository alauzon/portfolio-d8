<?php foreach ($images as $image_id => $image): ?>
  <div class="<?php print $image_classes[$image_id]; ?>" style="<?php print $image_styles[$image_id]; ?>">
    <?php print $image; ?>
  </div>
<?php endforeach; ?>
