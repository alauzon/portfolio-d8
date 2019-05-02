;
/**
 * A graph is a set nodes connected with edges.
 *
 * HTML given
 *
 * <.graphapi>
 *   <.graphapi-nodes> (<.graphapi-node><div.title><div.content></.graphapi-node)*
 *   <edges> (<edge>content</edge>)*
 * </>
 *
 * After setup
 * <.graphapi>
 *   <canvas>
 *   <.graphapi-nodes> (<.graphapi-node>)*
 *   <edges> (<edge>)*
 *   <.graphapi-menu>
 * </>
 *
 */
(function($) {
  $.fn.graphapi = function(options) {
    var opts = $.extend({}, $.graphapi.defaults, options);
    return this.each(function() {
      var $container = $(this);
      var options = {};
      options = $.extend({}, options, opts);
      $container.data('options', options).css('position', 'relative');
      
      var $canvas = $('<canvas>').prependTo($container);
      $canvas
      .css('backgroundColor', 'transparent')
      .width(opts.width)
      .height(opts.height)
      .css('position', 'absolute')
      .css('top', 0)
      .css('left', 0)
      ;
      $canvas.width(opts.width);
      $canvas.height(opts.height);

      var canvas = $canvas.get(0);
      canvas.width = opts.width;
      canvas.height = opts.height;

      $container.children('.graphapi-nodes')
      .width(opts.width)
      .height(opts.height)
      .css('overflow', 'hidden')
      .css('position', 'relative')
      .css('top', 0)
      .css('left', 0)
      ;

      if (opts.menu) {
        $.graphapi.menu($container);
      }
      $.graphapi.init($container);

      // TODO: http://paulirish.com/2011/requestanimationframe-for-smart-animating/
      setInterval(function() {
        $.graphapi.animate($container);
      }, 50);

    });

  };

  // Static members
  $.graphapi = {
    defaults : {
      width: 800,
      height: 600,
      backgroundColor: '#EEE',
      lineColor: '#000',
      arrowColor: '#222',

      menu : false,
      menuHide: true,
      showRegions : true,
      showForces : false,
      showLinks : false,
      animate: true,
      dragging: false,
      randomize: true,

      initScale: 2,

      applyAttractToCenter : true,
      applyBoundingBox: false,
      applyBoxOverlap: false,
      applyCoulombsLaw: true,
      applyDamping : true,
      applyHookesLaw : true,
      applyCompass : true,
      applyRank : false,

      attractToCenter : 200.0,
      boundingBox : 200.0,
      boxOverlap : 16.0,
      coulombsLaw : 256000.0,
      damping : 0.8,
      hookesLaw : 4.0,
      compass : 3.0,
      rankType : 'TopBottom',
      rankDepth : 8
    },

    // var a = [];jQuery('.graphapi-nodes > div').each(function() { a.push( jQuery(this).attr('rank'));}); a;
    doExport : function($container) {
      var text = 'digraph {';
      var $nodes = $container.children('.graphapi-nodes');
      $nodes.children().each(function(){
        text += this.id.replace(/-/g,'_') + " [";
        var $this = $(this);
        text += '  label = "' + $this.children('.graphapi-title').text() + '",';
        var position = $this.position();
        text += ' pos="' + position.left + ',' + position.top + '"'
        text += "]; \n"

      });
      var $edges = $container.children('edges');
      $edges.children().each(function(){
        var $this = $(this);
        text += ($this.attr('from').replace(/-/g,'_'));
        text += " ->" ;
        text += ($this.attr('to').replace(/-/g,'_'));
        text += " \n"

      });
      text += "}";
      //alert(text);
      //window.open('data:text/vnd-graphviz;charset=utf-8;filename=tada.txt,' + text,'_blank','height=300,width=400');
      window.open('data:text/vnd-graphviz;charset=utf-8;Content-Disposition: attachment; filename= graphapi.dot,' + text);

    },

    menu : function($container) {
      var m = $container.children('.graphapi-menu');
      var options = $container.data('options');

      if (m.size()==0) {
        m = $('<div>').addClass('graphapi-menu')
        .css('float', 'left')
        .css('backgroundColor', 'red')
        .css('z-index', 100)
        .css('position', 'absolute').css('top',0);
      ;
      }
      m.empty();
      
      $('<span>Menu</span>').appendTo(m);
      var l = $('<ul>').appendTo(m);

      var li = $('<li>');
      var cmd = $('<a href="#">Restart</a>').click(function(){
        $.graphapi.init($container);
        return false
      });
      cmd.appendTo(li);
      li.appendTo(l);

      li = $('<li>');
      cmd = $('<a href="#">Export</a>').click(function(){
        $.graphapi.doExport($container);
        return false
      });
      cmd.appendTo(li);
      li.appendTo(l);

      var checks = {
        animate : {
          label: 'Animate'
        },
        randomize : {
          label: 'Randomize'
        },
        showForces : {
          label: 'Show forces'
        },
        applyAttractToCenter : {
          label: 'Actract to center'
        },
        applyBoundingBox : {
          label: 'Bounding box'
        },
        applyBoxOverlap : {
          label: 'Box overlap'
        },
        applyCoulombsLaw : {
          label: 'Coulombs law'
        },
        applyDamping : {
          label: 'Damping'
        },
        applyCompass : {
          label: 'Compass'
        },
        applyHookesLaw : {
          label: 'Hookes law'
        },
        applyRank : {
          label: 'Apply Ranking'
        }
      }

      $.each(checks, function(key, opts){
        li = $('<li>');
        cmd = $('<input type="checkbox" />').click(function(){
          options[key] = this.checked;
        });
        cmd.appendTo(li);
        cmd.attr('checked', options[key]);
        $('<span>' + opts.label + '</span>').appendTo(li);
        li.appendTo(l);
      });

      if (options.menuHide) {
        m.hover(function(){
          l.slideDown();
        }, function(){
          l.slideUp();
        });
      }
      m.appendTo($container);
      l.slideUp();
    },

    init : function($container){
      var opts =  $container.data('options');
      $container.css('width', opts.width).css('height', opts.height);
      // setup nodes
      var $nodes = $container.children('.graphapi-nodes');
      $nodes.css('position', 'absolute').css('top',0)

      $nodes.children('div')
      .addClass('graphapi-node')
      .css('position', 'absolute')
      .each(function(index){
        var $this = $(this);
        if (opts.randomize) {
          $.graphapi.physics.init($this,
            (opts.initScale * (Math.random() - 1/2)) * opts.width + opts.width/2,
            (opts.initScale * (Math.random() - 1/2)) * opts.height + opts.height/2
            );
        }
      //TODO: add rank when missing attribute ... then we can rank by hand.
      //$this.attr('rank', '0');
      }).children('.graphapi-body').hide();

      // Move the edges into position
      if (opts.showLinks) {
        $container.children('edges').css({
          'position': 'absolute',
          'top': 0,
          'left': 0,
          'width':'100%'
        }).children().css('position', 'absolute');
      }
      else {
        $container.children('edges').children().css('display', 'none');

      }
    },

    canvas : {
      drawLine : function(ctx, physics1, physics2, color) {
        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.lineWidth = 1;
        ctx.moveTo(physics1.px, physics1.py);
        ctx.lineTo(physics2.px, physics2.py);
        ctx.stroke();
      },

      drawArrow : function(ctx, physics1, physics2, color) {
        var dirDx = physics2.px - physics1.px
        , dirDy = physics2.py - physics1.py;

        var p1x = physics1.px
        , p1y = physics1.py
        , ratio1 = Math.abs(dirDx) * physics1.dy - Math.abs(dirDy) * physics1.dx
        , temp
        ;


        if (ratio1 < 0) {
          // through top/bottom
          temp = physics1.dy * ((dirDy > 0) ? 1 : -1);
          p1x += temp * dirDx / dirDy;
          p1y += temp;
        }
        else if (ratio1 > 0) {
          // through left/right
          temp = physics1.dx * ((dirDx > 0) ? 1 : -1);
          p1y += temp * dirDy / dirDx;
          p1x += temp;
        }

        var p2x = physics2.px;
        var p2y = physics2.py;
        var ratio2 = Math.abs(dirDx) * physics2.dy - Math.abs(dirDy) * physics2.dx;
        if (ratio2 < 0) {
          // through top/bottom
          temp = physics2.dy * ((-dirDy > 0) ? 1 : -1);
          p2x += temp * dirDx / dirDy;
          p2y += temp;
        }
        else if (ratio2 > 0) {
          // through left/right
          temp = physics2.dx * ((-dirDx > 0) ? 1 : -1);
          p2y += temp * dirDy / dirDx;
          p2x += temp;
        }

        var w = Math.min(10.0, 5.0);
        var r2 = dirDx * dirDx + dirDy * dirDy;
        var r = Math.sqrt(r2);
        var forX = dirDx/r * w;
        var forY = dirDy/r * w;
        var leftX = -forY;
        var leftY = forX;
        var backX = forX * 0.5;
        var backY = forY * 0.5;

        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.fillStyle = color;
        ctx.moveTo(p1x,  p1y);
        ctx.lineTo(p2x,  p2y);
        ctx.lineTo(p2x - forX - leftX, p2y - forY - leftY);
        ctx.lineTo(p2x - forX + leftX, p2y - forY + leftY);
        ctx.lineTo(p2x ,  p2y);
        ctx.fill();
        ctx.stroke();
      },

      drawBox : function(ctx, physics) {
        ctx.beginPath();
        ctx.strokeStyle = '#000';
        ctx.strokeRect(physics.px - physics.dx, physics.py - physics.dy, 2* physics.dx, 2*physics.dy);
        ctx.stroke();
      }
    },
    
    physics : {
      init : function( $node, x, y) {
        var physics = {
          dx : $node.width() / 2,
          dy : $node.height() / 2,
          m : 1,
          q : 1,
          k : 1,
          px : x,
          py : y,
          vx : 0,
          vy : 0,
          ax : 0,
          ay : 0,
          fx : 0,
          fy : 0,
          id : $node.get(0).id
        }
        $node.data('physics', physics);
        $node.css('left', physics.px-physics.dx).css('top', physics.py-physics.dy);
      },

      applyForce : function ($node, fx, fy) {
        var physics = $node.data('physics');
        physics.fx += fx;
        physics.fy += fy;
      },

      updatePosition : function($node, dt, damping) {
        var physics = $node.data('physics');
        // F = m * a
        // da  = F / m * dt
        physics.ax = physics.fx / physics.m;
        physics.ay = physics.fy / physics.m;
        // dv = a * dt
        physics.vx += physics.ax * dt;
        physics.vy += physics.ay * dt;
        // Damping
        physics.vx *= damping;
        physics.vy *= damping;
        // dx = v * dt
        physics.px += physics.vx * dt;
        physics.py += physics.vy * dt;

        physics.o_fx = physics.fx;
        physics.o_fy = physics.fy;

        physics.fx = 0;
        physics.fy = 0;
        
        physics.dx = $node.width() / 2;
        physics.dy = $node.height() / 2;
        $node.css('left', physics.px - physics.dx).css('top', physics.py- physics.dy);
      },

      /**
       * Attract point to center
       */
      attractToCenter : function (physics, center, scale) {
        var dx = (center.px - physics.px);
        var dy = (center.py - physics.py);
        physics.fx += scale * dx / center.dx;
        physics.fy += scale * dy / center.dy;
      },

      /**
       * Prevent particle form escaping from container
       */
      boundingBox : function(particle, container, scale) {
        var dx = container.px - particle.px;
        var dy = container.py - particle.py;

        particle.fx += scale * dx / container.dx;
        particle.fy += scale * dy / container.dy;
      },

      /**
       * F = k (u-u0);
       */
      hookesLaw : function (physics1, physics2, scale) {
        var u0 = 40;
        var rx = physics1.px - physics2.px;
        var ry = physics1.py - physics2.py;
        var r2 = rx * rx + ry * ry;
        var r = Math.sqrt(r2);

        if (r < 0.01) r = 0.01;

        var f = (r-u0) * scale;
        
        var fx = f * rx/r;
        var fy = f * ry/r;
        

        physics1.fx+= -fx;
        physics1.fy+= -fy;

        physics2.fx+= fx;
        physics2.fy+= fy;

      },

      /**
       * F = q1 * q2 / r2
       */
      coulombsLaw : function (physics1, physics2, scale) {
        var rx = physics1.px - physics2.px;
        var ry = physics1.py - physics2.py;
        var r2 = rx * rx + ry * ry;
        if (r2 < 0.01) {
          r2 = 0.01;
        }
        var distance = Math.sqrt(r2);

        var fx = scale * (rx/distance) / r2
        var fy = scale * (ry/distance) / r2;

        physics1.fx += fx;
        physics1.fy += fy;

        physics2.fx -= fx;
        physics2.fy -= fy;
      },

      /**
       * If 2 particles overlap use there
       * borders to calculate overlap forces
       *
       */
      boxOverlap : function(physics1, physics2, scale) {
        var abs = Math.abs;
        var dx = physics2.px - physics1.px;
        var dy = physics2.py - physics1.py;
        var Dx = physics2.dx + physics1.dx + 30; // 30 px
        var Dy = physics2.dy + physics1.dy + 30; // 30 px

        var absdx = abs(dx);
        var absdy = abs(dy);

        if (absdx<Dx && absdy<Dy) {

          var overlapX = scale * (Dx - absdx);
          var overlapY = scale * (Dy - absdy);

          physics1.fx -= (dx > 0) ? overlapX : -overlapX;
          physics1.fy -= (dy > 0) ? overlapY : -overlapY;
          physics2.fx += (dx > 0) ? overlapX : -overlapX;
          physics2.fy += (dy > 0) ? overlapY : -overlapY;
        }
      },

      /**
       * Make to particles rotate like a compass needle
       *
       * The first is the south pole and the latter the north pole
       */
      compass : function(physics1, physics2, scale) {
        var dx = physics2.px - physics1.px;
        var dy = physics2.py - physics1.py;
        var Nx = dy;
        var Ny = -dx;

        if (dx > 0) {
          // Reverse force
          Nx = -Nx;
          Ny = -Ny;
        }

        physics1.fx += Nx * scale;
        physics1.fy += Ny * 0.9 * scale;

        physics2.fx -= Nx * scale;
        physics2.fy -= Ny * 0.9 * scale;
      },

      /**
       * Apply a ranking function on particle
       *
       * Only for positive ranking so we can disable ranking by
       * settings the ranking negative
       */
      rank : function(physics, physicsRoom, rankFunc, rankOrder, rankDepth, scale) {
        if (rankOrder <= 0) return;

        var f = rankFunc(physics, physicsRoom, rankOrder, rankDepth);

        physics.fx += f.fx * scale;
        physics.fy += f.fy * scale;
      }
    },

    rankTopBottom : function(physicsParticle, physicsRoom, rankOrder, rankDepth) {
      var my = 2 * physicsRoom.dy * (rankOrder) / (rankDepth+1) - physicsRoom.dy + physicsRoom.py;
      return {
        fx: 0,
        fy: my - physicsParticle.py
      };
    },

    rankBottomTop : function(physicsParticle, physicsRoom, rankOrder, rankDepth) {
      var my = 2 * physicsRoom.dy * (1 - (rankOrder-1) / rankDepth) - physicsRoom.dy + physicsRoom.py;
      return {
        fx: 0,
        fy: my - physicsParticle.py
      };
    },

    draw : function($container) {
      var opts = $container.data('options');
      var showForces = opts.showForces;
      var showLinks = opts.showLinks;
      var $nodes = $container.children('.graphapi-nodes');

      // TODO: this should draw c.q. update when dragging the current graph
      var canvas = $container.children('canvas').get(0)
      var ctx = canvas.getContext('2d');
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      // Update nodes
      $nodes.children('.graphapi-node').each(function() {
        var $node1 = $(this);
        var physics1 = $node1.data('physics');

        //$.graphapi.canvas.drawBox(ctx, physics1);
        if (showForces) {
          $.graphapi.canvas.drawLine(ctx, physics1, {
            px:physics1.px + physics1.o_fx,
            py:physics1.py + physics1.o_fy
          }, '#555');
        }
      });

      $container.children('edges').children().each(function() {
        var $this = $(this);
        var $from = $('#' + $this.attr('from'));
        var $to = $('#' + $this.attr('to'));
        var type = $this.attr('type');
        var physics1 = $from.data('physics');
        var physics2 = $to.data('physics');

        var color = $this.attr('color');
        if (typeof color == 'undefined') {
          color = '#000';
        }
        if (type == 'bi') {
          $.graphapi.canvas.drawLine(ctx, physics1, physics2, color);
        }
        else {
          $.graphapi.canvas.drawArrow(ctx, physics1, physics2, color);
        }
        if (showLinks) {
          $this.css('left', (physics1.px + physics2.px)/2).css('top', (physics1.py + physics2.py)/2);
        }
      });
    },

    animate : function ($container) {
      var opts = $container.data('options');
      if (opts.dragging) return;
      if (!opts.animate) return;

      var showForces = opts.showForces;

      var $nodes = $container.children('.graphapi-nodes');
      var width = $nodes.width();
      var height = $nodes.height();
      var lineColor = opts.lineColor;
      var arrowColor = opts.arrowColor;

      var applyAttractToCenter = opts.applyAttractToCenter;
      var applyBoundingBox = opts.applyBoundingBox;
      var applyBoxOverlap = opts.applyBoxOverlap;
      var applyCoulombsLaw = opts.applyCoulombsLaw;
      var applyDamping = opts.applyDamping;
      var applyHookesLaw = opts.applyHookesLaw;
      var applyCompass = opts.applyCompass;

      var attractToCenter = opts.attractToCenter;
      var boundingBox = opts.boundingBox;
      var boxOverlap = opts.boxOverlap;
      var coulombsLaw = opts.coulombsLaw;
      var damping = opts.damping;
      var hookesLaw = opts.hookesLaw;
      var compass = opts.compass;

      var fnAttractToCenter = $.graphapi.physics.attractToCenter;
      var fnBoundingBox = $.graphapi.physics.boundingBox;
      var fnBoxOverlap = $.graphapi.physics.boxOverlap;
      var fnCoulombsLaw = $.graphapi.physics.coulombsLaw;
      var fnHookesLaw = $.graphapi.physics.hookesLaw;
      var fnCompass = $.graphapi.physics.compass;

      // Rank settings
      var fnRank = $.graphapi.physics.rank;
      var rankType = opts.rankType;
      var rankDepth = opts.rankDepth;
      // TODO: validate type
      var fnRankType = $.graphapi[ 'rank' + rankType];
      var applyRank = opts.applyRank;
      if (rankDepth == 0) applyRank = false;

      var fnUpdatePosition = $.graphapi.physics.updatePosition;

      var damping = 1.00;
      if (applyDamping) {
        damping = opts.damping;
      }

      var center =  {
        px: width / 2,
        py: height / 2,
        dx: width / 2,
        dy: height / 2
      };

      // single point interaction
      $nodes.children('.graphapi-node').each(function() {
        var node1 = this;
        var $node1 = $(node1);
        var physics1 = $node1.data('physics');
        if (applyAttractToCenter) fnAttractToCenter(physics1, center, attractToCenter);
        if (applyBoundingBox) fnBoundingBox(physics1, center, boundingBox);

        if (applyRank) fnRank(physics1, center, fnRankType, $node1.attr('rank') % rankDepth, rankDepth, 1.0);

        // two point interaction
        $node1.nextAll('.graphapi-node').each(function() {
          var node2 = this;
          var $node2 = $(node2);
          if (node1.id != node2.id) {
            var physics2 = $node2.data('physics');
            if (applyCoulombsLaw) fnCoulombsLaw(physics1, physics2, coulombsLaw);
            if (applyBoxOverlap) fnBoxOverlap(physics1, physics2, boxOverlap);
          }
        });
      });

      $container.children('edges').children().each(function() {
        var $this = $(this);
        var from = '#' + $this.attr('from');
        var to = '#' + $this.attr('to');
        var physics1 = $(from).data('physics');
        var physics2 = $(to).data('physics');
        if (applyHookesLaw) fnHookesLaw(physics1, physics2, hookesLaw);
        if (applyCompass) fnCompass(physics1, physics2, compass);
      });

      // Update nodes
      $nodes.children('.graphapi-node').each(function() {
        fnUpdatePosition($(this), 0.050, damping);
      });

      $.graphapi.draw($container);
    }
  };
})(jQuery);

jQuery(document).ready(function(){
  var $ = jQuery;
  $('.graphapi').each(function(){
    var $container = $(this);
    $container.click(function() {
      $.graphapi.animate($container);
    });
  });
});
