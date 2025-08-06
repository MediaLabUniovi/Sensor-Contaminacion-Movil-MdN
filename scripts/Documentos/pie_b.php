<?php
//$textopie = cargartextos($link, "pie", $lang);
//$link->close();
?>

<!-- PIE DE PAGINA --> 
<footer id="contacto" class="footer-standard-dark bg-extra-dark-gray"> 
    <div class="footer-widget-area padding-five-tb sm-padding-30px-tb">
        <div class="container">
            <div class="row">
                <div class="col-lg-3 col-md-6 widget border-right border-color-medium-dark-gray md-no-border-right md-margin-30px-bottom sm-text-center">
                    <!-- start logo -->
                    <a href="index.php" class="margin-20px-bottom d-inline-block">
                        <img class="footer-logo" src="media/imagenes/logo-white.png" data-rjs="media/imagenes/logo-white@2x.png" alt="">
                    </a>
                    <!-- end logo -->

                    <ul class="latest-post position-relative">
                        <li class="media border-bottom border-color-medium-dark-gray">
                            <figure>
                                <a href="http://www.gijon.es"><img src="media/imagenes/Gijon_Blanco.svg" alt="logo Ayuntamiento de Gijón"></a>
                            </figure>
                        </li>
                    </ul>

                    <p class="text-small d-block margin-15px-bottom width-80 sm-width-100"><?= $textopie["t5"]; ?></p>
                    <div class="text-small">Email: <a href="mailto:medialab@uniovi.es">medialab@uniovi.es</a></div>
                    <div class="text-small"><?= $textopie["t6"]; ?>: +34 (0) 985 182 490</div>
                    <div class="text-small"><a href="https://goo.gl/maps/4nnmVDrg4t6Pawyg7" target="_blank"><?= $textopie["t7"]; ?></a></div>

                    <br>

                    
                    <a href="http://www.uniovi.es"><img src="media/imagenes/logo_uniovi.png" alt="logo Universidad de Oviedo"></a>
                </div>

                <!-- Contact Form -->
                <div class="col-lg-3 col-md-6 widget md-margin-30px-bottom">
                    <div class="widget-title alt-font text-small text-medium-gray text-uppercase margin-15px-bottom font-weight-600 text-center text-md-left"><?= $textopie["t14"]; ?></div>

                    <form id="suscripcion-form" action="avisos.php" method="post">
                        <div class="padding-fifteen-all bg-white border-radius-6 lg-padding-seven-all">
                            <input type="hidden" name="aviso" id="aviso" value="1">
                            <input type="text" name="nombre" id="nombre" placeholder="<?= $textopie["t15"]; ?>*" class="input-bg">
                            <input type="text" name="email" id="email" placeholder="<?= $textopie["t16"]; ?>*" class="input-bg">
                            <input type="text" name="interes" id="interes" placeholder="<?= $textopie["t17"]; ?>*" class="input-bg">      
                            <input type="text" id="honeypot" name="honeypot" style="display:none;">                             
                            <button id="subscribe-button" type="submit" class="btn btn-small border-radius-4 btn-black"><?= $textopie["t18"]; ?></button>
                        </div>
                    </form>
                </div>
                
                <!-- Spotify Embed -->
                <div class="col-lg-6 col-md-6 widget md-margin-30px-bottom">
                    <div class="widget-title alt-font text-small text-medium-gray text-uppercase margin-15px-bottom font-weight-600 text-center text-md-left"><?= $textopie["t19"]; ?></div>
                    <iframe style="border-radius:12px" src="https://open.spotify.com/embed/playlist/1VoEvOiS6QdGO2xuNGAICB?utm_source=generator" width="100%" height="450" frameborder="0" allow="autoplay; clipboard-write; encrypted-media; fullscreen; picture-in-picture" loading="lazy"></iframe>                
                     </div>
                        


            </div>
        </div>
    </div>
    
    <div class="bg-dark-footer padding-50px-tb text-center sm-padding-30px-tb">
        <div class="container">
            <div class="row">
                <div class="col-md-6 text-md-left text-small text-center">&copy; 2024 MediaLab ha utilizado la plantilla POFO (HTML) en la generación de este sitio web</div>
                <div class="col-md-6 text-md-right text-small text-center">
                    <p>version 1.1</p>
                </div>
            </div>
        </div>
    </div>
</footer>
