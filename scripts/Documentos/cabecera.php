<?
	$textocabecera=cargartextos ($link, "cabecera", $lang);
?>
       
<!-- start header -->
<header>
            
    <!-- Start navigation -->
    <nav class="navbar bg-transparent navbar-top navbar-transparent-no-sticky full-width-pull-menu white-link no-transition">
                    <div class="container-fluid nav-header-container height-100px padding-three-half-lr sm-height-70px sm-padding-15px-lr">
                        
                        <!-- Logo y redes sociales -->
                        <div class="header-social-icon border-none no-padding-left no-margin-left" style="width: 450px">
                            <a href="./index1.php?lang=<?php echo htmlspecialchars($lang); ?>" title="MediaLab" class="logo">
                                <img src="media/imagenes/logo.png" alt="Logo MediaLab" class="logo-dark">
                                <img src="media/imagenes/logo-white.png" alt="Logo MediaLab Light" class="logo-light default">
                            </a>
                            <a href="https://www.facebook.com/medialab_uniovi-829722250552959" title="Facebook" target="_blank"><i class="fab fa-facebook-f"></i></a>
                            <a href="https://twitter.com/medialab_uniovi" title="Twitter" target="_blank"><i class="fab fa-twitter"></i></a>
                            <a href="https://www.linkedin.com/company/medialab-universidad-de-oviedo" title="LinkedIn" target="_blank"><i class="fab fa-linkedin-in"></i></a>
                            <a href="https://www.instagram.com/medialab_uniovi/" title="Instagram" target="_blank"><i class="fab fa-instagram"></i></a>
                            <a href="https://www.youtube.com/channel/UCWdkndLLctAcNKrlSe_LLRQ" title="YouTube" target="_blank"><i class="fab fa-youtube"></i></a>                            
                            
                            <?php
                            // Obtenemos el nombre del archivo actual
                            $current_file = basename($_SERVER['PHP_SELF']);

                            // Verificamos si el archivo termina en "1" y lo reemplazamos por "0"
                            $text_version = preg_replace('/1\.php$/', '0.php', $current_file);

                            // Reconstruimos la URL con el archivo actualizado y los parámetros existentes
                            $text_url = $text_version . '?' . http_build_query($_GET);
                            ?>

                            <a href="<?php echo htmlspecialchars($text_url); ?>" title="only-text" target="_blank" class="text-extra-dark-gray text-deep-pink-hover margin-one-lr"><? echo $textocabecera["t9"];?></a>
                                                        
                            
                            </div>

                        <div class="col text-right pr-0">
                            <button class="navbar-toggler mobile-toggle d-inline-block" type="button" id="open-button" data-toggle="collapse" data-target=".navbar-collapse">
                                <span></span>
                                <span></span>
                                <span></span>
                            </button>
                            <div class="menu-wrap full-screen no-padding d-md-flex">
                                <div class="col-md-6 p-0 d-none d-md-block"> 
                                    <div class="cover-background full-screen">     
                                        <div class="opacity-light bg-extra-dark-gray"></div>
                                        <div class="position-absolute height-100 width-100 text-center">
                                            <div class="display-table height-100 width-100">
                                                <div class="display-table-cell height-100 width-100 vertical-align-middle position-relative">

                                                </div>
                                            </div>                                                            
                                        </div>                                                    
                                    </div>
                                </div>
                                <div class="col-md-6 p-0 bg-white full-screen text-left">
                                    <div class="position-absolute height-100 width-100 overflow-auto">
                                        <div class="display-table height-100 width-100">
                                            <div class="display-table-cell height-100 width-100 vertical-align-middle padding-fourteen-lr alt-font link-style-2 md-padding-seven-lr sm-padding-15px-lr">
                                                <!-- start menu -->
                                                <ul class="font-weight-600 sm-no-padding-left">
                                                    <!-- start menu item -->
                                                    <li>
                                                        <a href="transparencia.php" class="inner-link" title="<? echo $textocabecera["t1"];?>"><? echo $textocabecera["t1"];?></a>
                                                    </li>
                                                    <!-- end menu item -->
                                                    <!-- start menu item -->
                                                    <li>
                                                        <a href="./ods.php" class="inner-link" title="<? echo $textocabecera["t0"];?>"><? echo $textocabecera["t0"];?></a>
                                                    </li>
                                                    <!-- end menu item -->
                                                    <!-- start menu item -->
                                                    <li>
                                                        <a href="./personas.php" class="inner-link" title="<? echo $textocabecera["t2"];?>"><? echo $textocabecera["t2"];?></a>
                                                    </li>
                                                    <!-- end menu item -->
                                                    <li>
                                                        _____
                                                    </li>
                                                    <li>
                                                        <a href="./residencia.php" class="inner-link" title="Residencias 24-25">Residencia 24-25</a>
                                                    </li>
                                                    <!-- end menu item -->
                                                    <li>
                                                        _____
                                                    </li>
                                                    <!-- start menu item -->
                                                    <li>
                                                        <?php if (isset($_SESSION["id"])): ?>
                                                        <a href="https://www.medialab-uniovi.es/privado/logout.php" class="text-extra-dark-gray text-deep-pink-hover margin-one-lr line-height-normal" ><? echo $textocabecera["t4"];?></a>
                                                        <?php else: ?>
                                                        <a href="#contact-form" class="popup-with-form"><? echo $textocabecera["t3"];?></a>
                                                                <!-- start form -->
                                                                <form id="contact-form" action="./privado/login.php" method="post" class="white-popup-block mfp-hide col-lg-3 p-0 mx-auto">
                                                                    <div class="padding-fifteen-all bg-white border-radius-6 lg-padding-seven-all">
                                                                        <div class="text-extra-dark-gray alt-font text-large font-weight-600 margin-30px-bottom"><? echo $textocabecera["t5"];?></div>
                                                                        <div>
                                                                            <div id="success-contact-form" class="mx-0"></div>
                                                                            <input type="username" name="usuario" id="usuario" placeholder="<?php echo htmlspecialchars($textocabecera["t6"]); ?>*" class="input-bg">
                                                                            <input type="password" name="pwd" id="pwd" placeholder="<?php echo htmlspecialchars($textocabecera["t7"]); ?>*" class="input-bg">
                                                                            <button id="contact-us-button" type="submit" class="btn btn-small border-radius-4 btn-black"><?php echo htmlspecialchars($textocabecera["t8"]); ?></button>
                                                                        </div>
                                                                    </div>
                                                                </form>
                                                                <!-- end form -->
                                                        <?php endif; ?> 
                                                    </li>
                                                    
                                                    <!-- end menu item -->
                                                    
                                                    <?php
                                                    // Obtenemos la URL actual sin el parámetro de idioma
                                                    $current_url = $_SERVER['PHP_SELF'] . '?' . http_build_query(array_merge($_GET, ['lang' => '']));
                                                    ?>
                                                    
                                                    <li><a href="<?php echo htmlspecialchars($current_url) . "es"; ?>" title="<?php echo $textocabecera["t11"]; ?>" <?php if ($lang == "es") { ?>style="background-color: aquamarine;"<?php } ?>><span class="icon-country spain"></span><?php echo $textocabecera["t11"]; ?></a></li>
                                                    <li><a href="<?php echo htmlspecialchars($current_url) . "en"; ?>" title="<?php echo $textocabecera["t12"]; ?>" <?php if ($lang == "en") { ?>style="background-color: aquamarine;"<?php } ?>><span class="icon-country usa"></span><?php echo $textocabecera["t12"]; ?></a></li>
                                                    <li><a href="<?php echo htmlspecialchars($current_url) . "de"; ?>" title="<?php echo $textocabecera["t13"]; ?>" <?php if ($lang == "ale") { ?>style="background-color: aquamarine;"<?php } ?>><span class="icon-country deu"></span><?php echo $textocabecera["t13"]; ?></a></li>
                                                    <li><a href="<?php echo htmlspecialchars($current_url) . "it"; ?>" title="<?php echo $textocabecera["t14"]; ?>" <?php if ($lang == "ita") { ?>style="background-color: aquamarine;"<?php } ?>><span class="icon-country ita"></span><?php echo $textocabecera["t14"]; ?></a></li>
                                                    
                                                    
                                                </ul>
                                                <!-- end menu -->

                                                <!-- start social links -->
                                                <div class="margin-fifteen-top padding-35px-left sm-no-padding-left">
                                                    <div class="icon-social-medium margin-three-bottom">
                                                    <a href="https://www.facebook.com/medialab_uniovi-829722250552959/?ref=page_internal" title="Facebook" target="_blank" class="text-extra-dark-gray text-deep-pink-hover margin-one-lr"><i class="fab fa-facebook-f" aria-hidden="true"></i></a>
                                                    <a href="https://twitter.com/medialab_uniovi?lang=es" title="Twitter" target="_blank" class="text-extra-dark-gray text-deep-pink-hover margin-one-lr"><i class="fab fa-twitter"></i></a>
                                                    <a href="https://www.linkedin.com/company/medialab-universidad-de-oviedo" title="Linked In" target="_blank" class="text-extra-dark-gray text-deep-pink-hover margin-one-lr"><i class="fab fa-linkedin-in"></i></a>
                                                    <a href="https://www.instagram.com/medialab_uniovi/?hl=es" title="Instagram" target="_blank" class="text-extra-dark-gray text-deep-pink-hover margin-one-lr"><i class="fab fa-instagram"></i></a>                            
                                                    <a href="https://www.youtube.com/channel/UCWdkndLLctAcNKrlSe_LLRQ" title="YouTube" target="_blank" class="text-extra-dark-gray text-deep-pink-hover margin-one-lr"><i class="fab fa-youtube"></i></a>                            
                                                    </div>
                                                </div>
                                                <!-- start social links -->
                                            </div>
                                        </div>
                                    </div>
                                    <button class="close-button-menu" id="close-button"></button>
                                </div>
                            </div>
                        </div>
                        <!-- start menu -->
                    </div>
                    <!-- end header navigation -->
                </nav>
                <!-- end navigation -->

        <!-- end navigation --> 
            
        </header>
        <!-- end header -->
       