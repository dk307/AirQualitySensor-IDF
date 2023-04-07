
const path = require('path');
const gulp = require('gulp');
const through = require('through2');
const gzip = require('gulp-gzip');
const concat = require('gulp-concat');
const imagemin = require('gulp-imagemin');
const htmlvalidate = require('gulp-html');
var browserSync = require('browser-sync').create();
var reload = browserSync.reload;
var stripcomments = require('gulp-strip-comments');
var htmlminify = require('gulp-html-minifier-terser');
var minify = require('gulp-minify');
var shajs = require('sha.js');
var grun = require('gulp-run');
var log = require('fancy-log');
const sizeOfImage = require('image-size');


const baseFolder = 'static/';
const baseWebFolder = 'static/web';
const tempWebFolder = baseFolder + 'web/temp/data/';
const staticWebInclude = 'main/generated/web/include';
const staticDisplaySrc = 'main/generated/display/src';
const sdcardFolder = 'sdcard/';

// this converts to the header file
var toHeader = function(name, debug) {
  return through.obj(function(source, encoding, callback) {
    var parts = source.path.split(path.sep);
    var filename = parts[parts.length - 1];
    var safename = name || filename.split('.').join('_');

    var sha = shajs('sha256').update(source.contents).digest('hex');
    // console.info(sha);

    // Generate output
    var output = '';
    output += '#pragma once\n';
    output += '#define ' + safename + '_len ' + source.contents.length + '\n';
    output += 'const char ' + safename + '_sha256[] = "' + sha + '";\n';
    output += 'const uint8_t ' + safename + '[] = {';
    for (var i = 0; i < source.contents.length; i++) {
      if (i > 0) {
        output += ',';
      }
      if (0 === (i % 20)) {
        output += '\n';
      }
      output += '0x' + ('00' + source.contents[i].toString(16)).slice(-2);
    }
    output += '\n};';

    // clone the contents
    var destination = source.clone();
    destination.path = source.path + '.h';
    destination.contents = Buffer.from(output);

    if (debug) {
      console.info(
          'Image ' + filename + ' \tsize: ' + source.contents.length +
          ' bytes');
    }

    callback(null, destination);
  });
};

var toImageFile = function(name, debug) {
  return through.obj(function(source, encoding, callback) {
    var parts = source.path.split(path.sep);
    var filename = parts[parts.length - 1];
    var safename = name || filename.split('.').join('_');

    // Generate output
    var output = '';
    output += '#include <lvgl.h>\n';
    output += 'const uint8_t ' + safename + '_data[] = {';
    for (var i = 0; i < source.contents.length; i++) {
      if (i > 0) {
        output += ',';
      }
      if (0 === (i % 30)) {
        output += '\n';
      }
      output += '0x' + ('00' + source.contents[i].toString(16)).slice(-2);
    }

    const dimensions = sizeOfImage(source.path);

    output += '\n};\n\n';
    output += 'const lv_img_dsc_t ' + safename + '_img = {\n';
    output += '  .header.always_zero = 0,\n';
    output += '  .header.reserved = 0,\n';
    output += '  .header.cf = LV_IMG_CF_RAW_ALPHA,\n';
    output += '  .header.w = ' + dimensions.width + ',\n';
    output += '  .header.h = ' + dimensions.height + ',\n';
    output += '  .data_size = ' + source.contents.length + ',\n';
    output += '  .data = ' + safename + '_data\n';
    output += '};\n';

    // clone the contents
    var destination = source.clone();
    destination.path = source.path + '.c';
    destination.contents = Buffer.from(output);

    if (debug) {
      console.info(
          'Image ' + filename + ' \tsize: ' + source.contents.length +
          ' bytes');
    }

    callback(null, destination);
  });
};


gulp.task('html', function() {
  return gulp.src(baseFolder + 'web/*.html')
      .pipe(htmlvalidate())
      .pipe(htmlminify({
        collapseWhitespace: true,
        minifyJS: true,
        minifyCSS: true,
        removeComments: true,
        removeAttributeQuotes: true,
        collapseBooleanAttributes: true,
        collapseInlineTagWhitespace: true,
        decodeEntities: true,
        minifyURLs: true,
        removeEmptyAttributes: true,
        removeRedundantAttributes: true,
        sortAttributes: true,
        sortClassName: true
      }))
      .pipe(htmlvalidate())
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(toHeader(null, true))
      .pipe(gulp.dest(staticWebInclude));
});

gulp.task('js', function() {
  return gulp.src(baseFolder + 'web/js/*.js')
      .pipe(stripcomments())
      .pipe(concat('s.js'))
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(toHeader(null, true))
      .pipe(gulp.dest(staticWebInclude));
});

gulp.task('js-extra', function() {
  return gulp.src(baseFolder + 'web/js/extra/*.js')
      .pipe(stripcomments())
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(toHeader(null, true))
      .pipe(gulp.dest(staticWebInclude));
});

gulp.task('css', function() {
  return gulp.src(baseFolder + 'web/css/*.css')
      .pipe(minify())
      .pipe(gzip({gzipOptions: {level: 9}}))
      .pipe(gulp.dest(tempWebFolder))
      .pipe(toHeader(null, true))
      .pipe(gulp.dest(staticWebInclude));
});

gulp.task('web-images', function() {
  return gulp.src(baseFolder + 'web/media/*.png')
      .pipe(imagemin())
      .pipe(gulp.dest(tempWebFolder))
      .pipe(toHeader(null, true))
      .pipe(gulp.dest(staticWebInclude));
});

// display fonts start
function font_create(bpp, size, font, output, symbols_and_range) {
  return grun(
             'node ./node_modules/lv_font_conv/lv_font_conv.js --bpp ' + bpp +
             ' --size ' + size + ' --force-fast-kern-format --font ' + font +
             ' ' + symbols_and_range + ' --format lvgl  -o ' +
             staticDisplaySrc + '/' + output)
      .exec();
}

gulp.task('display-fonts-big-font', function() {
  return font_create(
      8, 165,
      './node_modules/@fontsource/montserrat/files/montserrat-all-300-normal.woff',
      'big_panel_font.c', '--symbols="0,1,2,3,4,5,6,7,8,9,-"');
});

gulp.task('display-fonts-temp-hum', function() {
  return font_create(
      4, 72,
      './node_modules/@fontsource/montserrat/files/montserrat-all-500-normal.woff',
      'temp_hum_font.c', '--symbols="0,1,2,3,4,5,6,7,8,9,F,C,°,⁒,-"');
});

gulp.task('display-fonts-40-regular-number', function() {
  return font_create(
      4, 40,
      './node_modules/@fontsource/montserrat/files/montserrat-all-400-normal.woff',
      'regular_numbers_40_font.c', '--symbols="0,1,2,3,4,5,6,7,8,9"');
});

gulp.task('display-fonts-48-all', function() {
  return font_create(
      4, 48,
      './node_modules/@fontsource/montserrat/files/montserrat-all-500-normal.woff',
      'all_48_font.c',
      '--range=0x20-0x7F --symbols="0,1,2,3,4,5,6,7,8,9,F,µ,g,/,m,³,°,F,⁒,p,-"');
});

gulp.task('display-fonts-18-uints', function() {
  return font_create(
      4, 18,
      './node_modules/@fontsource/montserrat/files/montserrat-all-500-normal.woff',
      'uints_18_font.c', '--symbols="C,F,µ,g,/,m,³,°,F,⁒,p,-,l,u,x,b"');
});

gulp.task('display-fonts-14-all', function() {
  return font_create(
      4, 14,
      './node_modules/@fontsource/montserrat/files/montserrat-all-500-normal.woff',
      'all_14_font.c',
      '--range=0x20-0x7F --symbols="0,1,2,3,4,5,6,7,8,9,F,µ,g,/,m,³,°,F,⁒,p,-"');
});

gulp.task(
    'display-fonts',
    gulp.series(
        'display-fonts-big-font', 'display-fonts-temp-hum',
        'display-fonts-40-regular-number', 'display-fonts-48-all',
        'display-fonts-18-uints', 'display-fonts-14-all'));

// display fonts end

gulp.task('display-images', function() {
  return gulp.src(baseFolder + 'display/image/*.png')
      .pipe(imagemin())
      .pipe(gulp.dest(tempWebFolder))
      .pipe(toImageFile(null, true))
      .pipe(gulp.dest(staticDisplaySrc));
});

gulp.task('display-file-copy', function() {
  return gulp.src(baseFolder + 'display/font/*.*')
      .pipe(gulp.dest(sdcardFolder + '/display/font'));
});

gulp.task(
    'default',
    gulp.series(
        'html', 'js', 'js-extra', 'css', 'web-images', 'display-fonts',
        'display-images', 'display-file-copy'));

gulp.task('serve', function() {
  browserSync.init({server: {baseDir: baseWebFolder}});
});

gulp.task('watchHtml', function() {
  gulp.watch(baseWebFolder + '**/*.html', reload)
});
gulp.task('watchJs', function() {
  gulp.watch(baseWebFolder + '**/*.js', reload)
});
gulp.task('watchCss', function() {
  gulp.watch(baseWebFolder + '**/*.css', reload)
});
gulp.task('watchPng', function() {
  gulp.watch(baseWebFolder + '**/*.png', reload)
});

// Static server
gulp.task(
    'webserver',
    gulp.parallel(['serve', 'watchHtml', 'watchJs', 'watchCss', 'watchPng']));
